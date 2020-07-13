# micro_server
simper cpp http server, form-data
uri::split_query #split the url params.
http_headers &reqHeaader = message.headers(); #get the all headers
std::string contentType = reqHeaader.find("Content-Type")->second; #find speical head
