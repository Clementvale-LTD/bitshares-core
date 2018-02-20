import sys
if sys.version_info.major == 3:
  from urllib.request import HTTPPasswordMgrWithDefaultRealm, HTTPBasicAuthHandler, Request, build_opener
  from urllib.parse import urlencode
else:
  from urllib2 import HTTPPasswordMgrWithDefaultRealm, HTTPBasicAuthHandler, Request, build_opener
  from urllib import urlencode


def curl(url, params=None, auth=None, req_type="GET", data=None, headers=None):
  post_req = ["POST", "PUT"]
  get_req = ["GET", "DELETE"]

  if params is not None:
    url += "?" + urlencode(params)

  if req_type not in post_req + get_req:
    raise IOError("Wrong request type \"%s\" passed" % req_type)

  _headers = {}
  handler_chain = []

  if auth is not None:
    manager = HTTPPasswordMgrWithDefaultRealm()
    manager.add_password(None, url, auth["user"], auth["pass"])
    handler_chain.append(HTTPBasicAuthHandler(manager))

  if req_type in post_req and data is not None:
    _headers["Content-Length"] = len(data)

  if headers is not None:
    _headers.update(headers)

  director = build_opener(*handler_chain)

  if req_type in post_req:
    if sys.version_info.major == 3:
      _data = bytes(data, encoding='utf8')
    else:
      _data = bytes(data)

    req = Request(url, headers=_headers, data=_data)
  else:
    req = Request(url, headers=_headers)

  req.get_method = lambda: req_type
  result = director.open(req)

  return {
    "httpcode": result.code,
    "headers": result.info(),
    "content": result.read()
  }
