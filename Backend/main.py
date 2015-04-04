import pebbleflipapi

def application(environ, start_response):

	raw_returned = pebbleflipapi.process_request(environ)

	returned = str(raw_returned[0])

	content_type = raw_returned[1]	

	response_headers = [('Content-Type', content_type)]

	start_response('200 OK', response_headers)

	return returned
