from instagram.client import InstagramAPI
from PIL import Image
from StringIO import StringIO
import urllib2 as urllib
import io
import json

CLIENT_ID = '<CLIENT_ID>'
CLIENT_SECRET = '<CLIENT_SECRET>'
REDIRECT_URI = 'https://www.dvappel.me/flip/authenticate'

scope = ["basic"]

api = InstagramAPI(client_id=CLIENT_ID, client_secret=CLIENT_SECRET, redirect_uri=REDIRECT_URI)


def process_request(environ):

	path = environ['PATH_INFO']

	request = path.split('/')
	#necessary as a temporary variable since you can't get length of an unassigned list
	request = request[len(request) - 1]

	# example path
	# /flip/authenticate
	# path would be ['', 'flip', 'authenticate']
	# looking for 'authenticate'

	# Lots of extras in here... looks pretty ugly but its pretty simple and works alright - might remove in the future

	if request == 'authenticate':
		return (do_authentication(environ), 'text/html')
	elif request == 'redirect':
		return (get_redirect_uri(), 'text/html')
	elif request == 'media_feed':
		return (get_media_feed(environ, 'json'), 'application/json')
	elif request == 'media_feed_id_list':
		return (get_media_feed_url_list(environ, 'media_id_list'), 'application/json')
	elif request == 'media_feed_id_list_with_prefix':
		return (get_media_feed_url_list(environ, 'media_id_list_with_prefix'), 'application/json')
	elif request == 'media_feed_id_list_with_prefix_and_postfix':
		return (get_media_feed_url_list(environ, 'media_id_list_with_prefix_and_postfix'), 'application/json')
	elif request == 'media_feed_url_list':
		return (get_media_feed_url_list(environ, 'url_list'), 'application/json')
	elif request == 'media':
		return (get_media(environ), 'image/png')
	elif request == 'log':
		return (get_log(), 'text/html')
	elif request == 'clear':
		return (clear_log(), 'text/html')
	else:
		return 'Error: invalid request URL'

	return environ


# Start /media_feed_url_list requests

def get_media_feed_url_list(environ, action):

	media_feed_returned = get_media_feed(environ, 'raw')
	# Since I kind of screwed this method on after I did the rest, the if statement part is a little clunky
	# media_feed_returned is [<media feed>, <access token>]

	media_feed = media_feed_returned[0]
	access_token = media_feed_returned[1]

	media_feed_url_list = []

	for media in media_feed:


		if action == 'url_list':
			media_url = get_url_from_media(media)
			media_url = construct_media_url(media.id, access_token)
			media_feed_url_list.append(media_url)
		elif action == 'media_id_list':
			if check_media_type(media):
				media_url = media.id
				media_feed_url_list.append(media_url)
		elif action == 'media_id_list_with_prefix':
			if check_media_type(media):
				media_url = 'media_id=' + media.id
				media_feed_url_list.append(media_url)
		elif action == 'media_id_list_with_prefix_and_postfix':
			if check_media_type(media):
				media_url = 'media_id=' + media.id + '&token='
				media_feed_url_list.append(media_url)
		else:
			print action


	return json.dumps(map(str, media_feed_url_list))


def check_media_type(media):
	if media.type == 'image':
		return True
	else:
		return False

def construct_media_url(media_id, access_token):

	media_url_base = 'https://www.dvappel.me/flip/media?media_id='

	media_url_middle = '&token='

	media_url = media_url_base + media_id + media_url_middle + access_token

	return media_url

# End /media_feed_url_list requests

# Start /log requests

def get_log():

	log_final = '<html> <head> </head> <body>'

	log = open('uwsgi_log', 'r')
	log = log.readlines()

	log = [x.replace('\n','') for x in log]

	log = log[::-1]

	log = '<br>'.join(log)

	log_final += log
	log_final += '</body> </html>'

	return log_final

def clear_log():

	log = open('uwsgi_log', 'w')
	log.write('')
	log.close()

	return redirect_template('https://www.dvappel.me/flip/log')
# End /log requests


# Start /media requests

def get_media(environ):

	media_id = get_media_id(environ)

	access_token = get_access_token_from_environ(environ)

	media = get_media_from_id(media_id, access_token)

	media_url = get_url_from_media(media)

	raw_image = get_image_from_url(media_url)

	image = process_raw_image(raw_image)
	# download image from url (DONE)
	# rework image to fit pebble
	# return that image

	# return media_url
	return image


def get_media_id(environ):

	query_string = environ['QUERY_STRING']

	if query_string is not '':

		query_params = parse_params_from_query_string(query_string)

		# where query_params looks like:
		# [['media_id', '<media id>'], ['token', '<access token>']]

		media_id = query_params[0][1]

		return media_id
	else:
		raise ValueError('Error: Missing or invalid media id')
		return

def get_access_token_from_environ(environ):

	query_string = environ['QUERY_STRING']

	if query_string is not '':

		query_params = parse_params_from_query_string(query_string)

		# where query_params looks like:
		# [['media_id', '<media id>'], ['token', '<access token>']]

		return query_params[1][1]
	else:
		raise ValueError('Error: Missing or invalid access token')
		return



def get_media_from_id(media_id, access_token):

	authed_api = InstagramAPI(access_token=access_token)

	media = authed_api.media(media_id)

	return media


def get_url_from_media(media):

	if media.type == 'image':
		photo_url = media.get_thumbnail_url()
		return photo_url
	else:
		raise ValueError('Error: Media is a video, not an image')
		return

def get_image_from_url(url):

	raw_image = urllib.urlopen(url)

	image_file = io.BytesIO(raw_image.read())
	image = Image.open(image_file)
	# Opens file from url, loads it into a BytesIO object, then opens that object as an image

	return image


def process_raw_image(raw_image):

	raw_image = raw_image.convert('1')
	raw_image = raw_image.resize((120,120), Image.ANTIALIAS)

	final_image = StringIO()
	# creates a StringIO object for proper display

	raw_image.save(final_image, 'PNG')
	# saves the image so that it can actually be returned & displayed


	return final_image.getvalue()

# End /media requests

# Start /media_feed requests

def get_media_feed(environ, format):

	query_string = environ['QUERY_STRING']

	if query_string is not '':

		query_params = parse_params_from_query_string(query_string)

		access_token = query_params[0][1]

		authed_api = InstagramAPI(access_token=access_token)

		media_feed = authed_api.user_media_feed()

		media_feed = media_feed[0]

		[str(i) for i in media_feed]

		if format == 'json':
			return json.dumps(map(str, media_feed))
		elif format == 'raw':
			return (media_feed, access_token)
	else:
		return 'Error: missing access token'

def parse_params_from_query_string(query_string):

	query_string = query_string.split('&')

	query_params = []

	for param in query_string:

		split_param = param.split('=')
		query_params.append(split_param)

	return query_params

# End /media_feed requests


# Start /redirect & /authenticate requests

def do_authentication(environ):

	query_string = environ['QUERY_STRING']

	if query_string is not '':

		query = query_string.split('=')
		access_token_code = query[1]

		base_url = 'pebblejs://close\#'

		access_token = get_access_token_from_code(access_token_code)

		full_url = base_url + access_token

		return redirect_template(full_url)

	else:
		return 'Error: missing request parameters in URL'



def get_access_token_from_code(code):

	if len(code) is 32:
		access_token_raw = api.exchange_code_for_access_token(code)
		access_token = access_token_raw[0]
		#access_token_raw contains a lot of useful information (really its just the basic user info):
		#(u'<access token> (as a string)', {u'username': u'<username>', u'bio': u'<bio>', u'website': u'<website>', u'profile_picture': u'<profile picture url>', u'full_name': u'<full name>', u'id': u'<user id>'})
		#but only the access token is needed. It's stored as the first element in access_token_raw.

		return access_token
	else:
		return 'Error: missing or invalid code parameter in URL'


def get_redirect_uri():

	redirect_uri = api.get_authorize_login_url(scope = scope)

	return redirect_template(redirect_uri)


def redirect_template(url):

	return '<META http-equiv="refresh" content="0;URL=' + url + '">'

# End /redirect & /authenticate requests
