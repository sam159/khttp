# KHttpd

This is a threaded (pthreads) HTTP 1.1 compliant server. It was created as an excercise in learning more about C.

Not intended for production, it has not had sufficient testing to ensure that it is secure and reliable.

## Libs

Other than Pthreads and Magic (for content type detection) the only external code is from [Joyend Http Parsing lib][1]
for parsing http requests and [UTHash][2] for managing linked lists, arrays and strings.

[1]: https://github.com/joyent/http-parser
[2]: https://troydhanson.github.io/uthash/
