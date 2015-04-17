#include <pebble.h>
#include "netdownload.h"
#ifdef PBL_PLATFORM_APLITE
#include "png.h"
#endif

// Keys
#define IMAGE_DATA 1
		
#define IMAGE_DATA_RECEIVED 5
	
#define IMAGE_DATA_DL_LENGTH 6

#define IMAGE_DATA_DL_STOP 7
	
#define ACCESS_TOKEN 2
	
#define ACCESS_TOKEN_SAVE 10
	
#define ACCESS_TOKEN_RECEIVED 9
	
#define GET_USERNAME 3
	
#define SENT_USERNAME 4
	
#define PEBBLE_JS_READY 0

	
	
static Window *window;
static TextLayer *text_layer;
static BitmapLayer *bitmap_layer;
static GBitmap *current_bmp;

char *images[20];
char *base_url = "https://www.dvappel.me/flip/media?";
char access_token[51];
//= "611228313.b7f2f87.c963add7547b466ea1642a9c0f188dc3";

int curr_image = 0;

size_t images_length;
bool got_all_images = false;



static unsigned long image = 0;


void send_app_message_requesting_username(char* curr_url) {
	DictionaryIterator *outbox;
	app_message_outbox_begin(&outbox);
	
	dict_write_cstring(outbox, GET_USERNAME, curr_url);
	
	app_message_outbox_send();
}

void show_next_image_step_1() {
	
  // show that we are loading by showing no image
  bitmap_layer_set_bitmap(bitmap_layer, NULL);

  text_layer_set_text(text_layer, "Loading...");

  // Unload the current image if we had one and save a pointer to this one
  if (current_bmp) {
    gbitmap_destroy(current_bmp);
    current_bmp = NULL;
  }

	APP_LOG(APP_LOG_LEVEL_DEBUG, "From show next image: %s", images[image]);
	
	send_app_message_requesting_username(images[image]);
}

void show_next_image_step_2(char* username) {
  //netdownload_request(images[image]);
	netdownload_request("http://assets.getpebble.com.s3-website-us-east-1.amazonaws.com/pebble-faces/cherie.png");
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	TextLayer *username_text_layer = text_layer_create((GRect) { .origin = { 168, 0 }, .size = { bounds.size.w, 20, } });
	
	text_layer_set_text(username_text_layer, username);
	text_layer_set_text_alignment(username_text_layer, GTextAlignmentLeft);

	layer_add_child(window_layer, text_layer_get_layer(username_text_layer));
  
  image++;
  if (image >= images_length/sizeof(char*)) {
    image = 0;
  }
}

void show_prev_image() {
	if( image == 0){
		show_next_image_step_1();
	} else if( image == 1){
		image = image - 1;
		show_next_image_step_1();
	} else {
		image = image - 2;
	}
	//show_next_image_step_1();
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create((GRect) { .origin = { 0, 72 }, .size = { bounds.size.w, 50, } });
	text_layer_set_overflow_mode(text_layer, GTextOverflowModeWordWrap);
  text_layer_set_text(text_layer, "Pebble-flip is loading... Please Wait");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  bitmap_layer = bitmap_layer_create(bounds);
  layer_add_child(window_layer, bitmap_layer_get_layer(bitmap_layer));
  current_bmp = NULL;
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
  bitmap_layer_destroy(bitmap_layer);
  gbitmap_destroy(current_bmp);
}

void download_complete_handler(NetDownload *download) {
  printf("Loaded image with %lu bytes", download->length);
  printf("Heap free is %u bytes", heap_bytes_free());

  #ifdef PBL_PLATFORM_APLITE
  GBitmap *bmp = gbitmap_create_with_png_data(download->data, download->length);
  #else
    GBitmap *bmp = gbitmap_create_from_png_data(download->data, download->length);
  #endif
  bitmap_layer_set_bitmap(bitmap_layer, bmp);

  // Save pointer to currently shown bitmap (to free it)
  if (current_bmp) {
    gbitmap_destroy(current_bmp);
  }
  current_bmp = bmp;

  // Free the memory now
  #ifdef PBL_PLATFORM_APLITE
  // gbitmap_create_with_png_data will free download->data
  #else
    free(download->data);
  #endif
  // We null it out now to avoid a double free
  download->data = NULL;
  netdownload_destroy(download);
}






char* concat(char *s1, char *s2) {
    char *result = malloc(strlen(s1)+strlen(s2)+1);
	
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

void process_image_data(char *image_data){
	
	APP_LOG(APP_LOG_LEVEL_INFO, "image 123: %s", image_data);
	
	char *full_url = concat(base_url, image_data);
	full_url = concat(full_url, access_token);
	//full_url = "http://assets.getpebble.com.s3-website-us-east-1.amazonaws.com/pebble-faces/cherie.png";
	images[curr_image] = full_url;
	APP_LOG(APP_LOG_LEVEL_INFO, "images: %s", images[curr_image]);
	
}



void send_app_message_received() {
	DictionaryIterator *outbox;
	app_message_outbox_begin(&outbox);

	dict_write_cstring(outbox, IMAGE_DATA_RECEIVED, "IMAGE_DATA_RECEIVED");

	app_message_outbox_send();
}

void save_access_token(char *access_token) {
	persist_write_string(ACCESS_TOKEN_SAVE, access_token);
}

void send_message_access_token_received(){
	DictionaryIterator *outbox;
	app_message_outbox_begin(&outbox);
	// Tell the javascript how big we want each chunk of data: max possible size - dictionary overhead with one Tuple in it.
	//uint32_t chunk_size = app_message_inbox_size_maximum() - dict_calc_buffer_size(1);
	//dict_write_int(outbox, NETDL_CHUNK_SIZE, &chunk_size, sizeof(uint32_t), false);
	// Send the URL
	
	//access_token = "611228313.b7f2f87.c963add7547b466ea1642a9c0f188dc3";
	
	dict_write_cstring(outbox, ACCESS_TOKEN_RECEIVED, access_token);
	
	APP_LOG(APP_LOG_LEVEL_INFO, "access token read from memory: %s", access_token);

	app_message_outbox_send();
}




void flip_message_receive(DictionaryIterator *iter, void *context) {
	//NetDownloadContext *ctx = (NetDownloadContext*) context;

	Tuple *tuple = dict_read_first(iter);
	if (!tuple) {
		//APP_LOG(APP_LOG_LEVEL_ERROR, "Got a message with no first key! Size of message: %li", (uint32_t)iter->end - (uint32_t)iter->dictionary);
		APP_LOG(APP_LOG_LEVEL_ERROR, "Got a message with no first key!");
		return;
	}
	
	switch (tuple->key) {
		case ACCESS_TOKEN:
			APP_LOG(APP_LOG_LEVEL_INFO, "Received access token as : %s", (char *)tuple->value);
			save_access_token((char *)tuple->value);
			break;
	  
		case IMAGE_DATA_DL_STOP:
			APP_LOG(APP_LOG_LEVEL_INFO, "Got all images");
			text_layer_set_text(text_layer, "Done! Press the down button to load the first image in your feed.");
		
			break;
	  
		case IMAGE_DATA_DL_LENGTH:
		  	APP_LOG(APP_LOG_LEVEL_DEBUG, "Image data length: %d", (int)tuple->value);
			images_length = (int)tuple->value;
			//text_layer_set_text(text_layer, "Initializing... (Don't press anything!)");
		  	send_app_message_received();
			break;
	  
		case IMAGE_DATA:
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Got IMAGE_DATA %s", (char *)tuple->value);
		  	process_image_data((char *)tuple->value);
		  	curr_image++;
			send_app_message_received();
			break;
		
		case SENT_USERNAME:
			APP_LOG(APP_LOG_LEVEL_INFO, "Got username as %s", (char *)tuple->value);
			char* received_username = (char *)tuple->value;	
			show_next_image_step_2(received_username);
			break;
		
		case PEBBLE_JS_READY:
			APP_LOG(APP_LOG_LEVEL_INFO, "Got pebblejs ready");
			send_message_access_token_received();
			break;

		
		default:
			netdownload_receive(iter, context);
			//send_message_access_token_received();
			break;
	}
}








void tap_handler(AccelAxisType accel, int32_t direction) {
	// show_next_image_step_1();
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
	show_prev_image();
}
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
	show_next_image_step_1();
}

static void click_config_provider(void *context) {
  // Register the ClickHandlers
	window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}





static void deinit(void) {
  netdownload_deinitialize(); // call this to avoid 20B memory leak
  window_destroy(window);
}

static void init(void) {
  // Need to initialize this first to make sure it is there when
  // the window_load function is called by window_stack_push.
  netdownload_initialize(download_complete_handler);

  window = window_create();
  window_set_fullscreen(window, true);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
  
  window_set_click_config_provider(window, click_config_provider);
	
  accel_tap_service_subscribe(tap_handler);

 
	app_message_register_inbox_received(flip_message_receive);

	
	persist_read_string(ACCESS_TOKEN_SAVE, access_token, sizeof(access_token));

	/*
	if(persist_exists(ACCESS_TOKEN_SAVE)) {
		persist_read_string(ACCESS_TOKEN_SAVE, access_token, sizeof(access_token));
		send_message_access_token_received();
	} else {
		send_message_access_token_received();
	}
	*/
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}