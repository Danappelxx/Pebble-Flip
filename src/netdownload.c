#include "netdownload.h"

static TextLayer *text_layer;
	
char *images[20]; /*= {
  //"https://www.dvappel.me/flip/media?media_id=954006307469480285_1374598283&token=611228313.b7f2f87.c963add7547b466ea1642a9c0f188dc3"
};
*/

// char **temp_images;

char *base_url = "https://www.dvappel.me/flip/media?";
char access_token[51];// = "&token=611228313.b7f2f87.c963add7547b466ea1642a9c0f188dc3";

int curr_image = 0;

// size_t images_length = sizeof(images);
size_t images_length;
bool got_all_images = false;

char* concat(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);
	
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

/*
void append_url_to_images_list(char *url) {
	APP_LOG(APP_LOG_LEVEL_INFO, "final url (cut-off): %s", url);

	images = temp_images;
}
*/

/*
void finalize_images_urls() {
	
}
*/	
void process_image_data(char *image_data){
	
	APP_LOG(APP_LOG_LEVEL_INFO, "image 123: %s", image_data);
	
	char *full_url = concat(base_url, image_data);
	full_url = concat(full_url, access_token);
	
	images[curr_image] = full_url;
	APP_LOG(APP_LOG_LEVEL_INFO, "images: %s", images[curr_image]);
	
}

void send_app_message_received() {
	DictionaryIterator *outbox;
	app_message_outbox_begin(&outbox);
	// Tell the javascript how big we want each chunk of data: max possible size - dictionary overhead with one Tuple in it.
	//uint32_t chunk_size = app_message_inbox_size_maximum() - dict_calc_buffer_size(1);
	//dict_write_int(outbox, NETDL_CHUNK_SIZE, &chunk_size, sizeof(uint32_t), false);
	// Send the URL
	dict_write_cstring(outbox, IMAGE_DATA_RECEIVED, "IMAGE_DATA_RECEIVED123");

	app_message_outbox_send();
}

void save_access_token(char *access_token) {
	persist_write_string(ACCESS_TOKEN_SAVE, access_token);
}




NetDownloadContext* netdownload_create_context(NetDownloadCallback callback) {
  NetDownloadContext *ctx = malloc(sizeof(NetDownloadContext));

  ctx->length = 0;
  ctx->index = 0;
  ctx->data = NULL;
  ctx->callback = callback;

  return ctx;
}

void netdownload_destroy_context(NetDownloadContext *ctx) {
  if (ctx->data) {
    free(ctx->data);
  }
  free(ctx);
}

void netdownload_initialize(NetDownloadCallback callback) {
  NetDownloadContext *ctx = netdownload_create_context(callback);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "NetDownloadContext = %p", ctx);
  app_message_set_context(ctx);

  app_message_register_inbox_received(netdownload_receive);
  app_message_register_inbox_dropped(netdownload_dropped);
  app_message_register_outbox_sent(netdownload_out_success);
  app_message_register_outbox_failed(netdownload_out_failed);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Max buffer sizes are %li / %li", app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

void netdownload_deinitialize() {
  netdownload_destroy_context(app_message_get_context());
  app_message_set_context(NULL);
}

void netdownload_request(char *url) {
	APP_LOG(APP_LOG_LEVEL_INFO, "netdl url: %s", url);
  DictionaryIterator *outbox;
  app_message_outbox_begin(&outbox);
  // Tell the javascript how big we want each chunk of data: max possible size - dictionary overhead with one Tuple in it.
  uint32_t chunk_size = app_message_inbox_size_maximum() - dict_calc_buffer_size(1);
  dict_write_int(outbox, NETDL_CHUNK_SIZE, &chunk_size, sizeof(uint32_t), false);
  // Send the URL
  dict_write_cstring(outbox, NETDL_URL, url);

  app_message_outbox_send();
}

void netdownload_destroy(NetDownload *image) {
  // We malloc'd that memory before creating the GBitmap
  // We are responsible for freeing it.
  if (image) {
    free(image->data);
    free(image);
  }
}







void netdownload_receive(DictionaryIterator *iter, void *context) {
  NetDownloadContext *ctx = (NetDownloadContext*) context;

  Tuple *tuple = dict_read_first(iter);
  if (!tuple) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Got a message with no first key! Size of message: %li", (uint32_t)iter->end - (uint32_t)iter->dictionary);
    return;
  }
  switch (tuple->key) {
	  
	case ACCESS_TOKEN:
		APP_LOG(APP_LOG_LEVEL_INFO, "Received access token as : %s", (char *)tuple->value);
		save_access_token((char *)tuple->value);
		break;
	  
	case IMAGE_DATA_DL_STOP:
		APP_LOG(APP_LOG_LEVEL_INFO, "Got all images");
		//text_layer_set_text(text_layer, "Done! Press the down button to load the first image in your feed.");
		
		break;
	  
	case IMAGE_DATA_DL_LENGTH:
	  	APP_LOG(APP_LOG_LEVEL_DEBUG, "Image data length: %d", (int)tuple->value);
		images_length = (int)tuple->value;
		//text_layer_set_text(text_layer, "Initializing... (Don't press anything)");
	  	send_app_message_received();
		break;
	  
	case IMAGE_DATA:
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Got IMAGE_DATA %s", (char *)tuple->value);
	  	process_image_data((char *)tuple->value);
	  	curr_image++;
		send_app_message_received();
		break;
	  
	  
	  
    case NETDL_DATA:
      if (ctx->index + tuple->length <= ctx->length) {
        memcpy(ctx->data + ctx->index, tuple->value->data, tuple->length);
        ctx->index += tuple->length;
      }
      else {
        APP_LOG(APP_LOG_LEVEL_WARNING, "Not overriding rx buffer. Bufsize=%li BufIndex=%li DataLen=%i",
          ctx->length, ctx->index, tuple->length);
      }
      break;
    case NETDL_BEGIN:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Start transmission. Size=%lu", tuple->value->uint32);
      if (ctx->data != NULL) {
        free(ctx->data);
      }
      ctx->data = malloc(tuple->value->uint32);
      if (ctx->data != NULL) {
        ctx->length = tuple->value->uint32;
        ctx->index = 0;
      }
      else {
        APP_LOG(APP_LOG_LEVEL_WARNING, "Unable to allocate memory to receive image.");
        ctx->length = 0;
        ctx->index = 0;
      }
      break;
    case NETDL_END:
      if (ctx->data && ctx->length > 0 && ctx->index > 0) {
        NetDownload *image = malloc(sizeof(NetDownload));
        image->data = ctx->data;
        image->length = ctx->length;

        printf("Received file of size=%lu and address=%p", ctx->length, ctx->data);
        ctx->callback(image);

        // We have transfered ownership of this memory to the app. Make sure we dont free it.
        // (see netdownload_destroy for cleanup)
        ctx->data = NULL;
        ctx->index = ctx->length = 0;
      }
      else {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Got End message but we have no image...");
      }
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_WARNING, "Unknown key in dict: %lu", tuple->key);
      break;
  }
}

char *translate_error(AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "UNKNOWN ERROR";
  }
}

void netdownload_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Dropped message! Reason given: %s", translate_error(reason));
}

void netdownload_out_success(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Message sent.");
}

void netdownload_out_failed(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to send message. Reason = %s", translate_error(reason));
}