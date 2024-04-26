#ifndef FLASH_WRITE_RSA_KEY
#define FLASH_WRITE_RSA_KEY

void flash_writer();
size_t flash_reader(uint8_t *decrypted_rsa_private_key,size_t flash_size);
size_t string_with_AES(uint8_t *input_string,size_t length,bool mode);
void print_hex_uint8_t(uint8_t *str, size_t size);

#define TESTING_RSA_PRIVATE_KEY "-----BEGIN PRIVATE KEY-----\n\
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDY7J/zoDOKNC92\n\
gA85PHHCT02aCAp9b5l2PQRIgaefagnUQyEOa/W91h4Q0dt3St4bwkqENzHDsvCC\n\
2zKqw6SYcbMFR8JFSBFw/e1SGFnqgDCbrxbdDMcypWeVJcS6VLnTpY6tUYNNP7o+\n\
AoKwjJFQZNmCGXwcrkK0lUo9wtq1bUWqi/ffJwP5S9fTVbS1DNW0Ppql06Aoaasv\n\
1QmNce/wzUyXx0wDKErxh8H+zg0NVLcx2aRhlcKajq6MT97bB8eDW3XcjdMBb+Nl\n\
+8FCag8lk9p8DakWY4bI/IjhWr57E7Mr9XZga+vWpzdaLoIV/96HSJ9JGUQGGvey\n\
RN5kTQQ/AgMBAAECggEBALE2/qVlZzWs1GmWzNkRjE4Vw2vmzcsT9OcniexCH9KY\n\
KleCT+scP7JKzl0nVIfsXe+HlqDVBo6+DMaalJX1Ju5zVLEnooo/CiCpEcxToIF3\n\
ID7Pl3oXClVR04GBTWp79SP2yMzHHuFpApnOKz5YWA65lQg4EbDMnCozHGfAXDSC\n\
A0hZfRSUjjBM6Ej25HvWEsDYKmF/6unwOUfxCkWGcx9ouCav9rpQvftHwseGASk7\n\
X6YOT79RM080baz6XKkmDdvHxKK7w3Bd+hmdaLZhjwc49p3ktS0iFl/SGuMg6dwY\n\
zlF5TK9RYrv7B1o6YWy6sQqpnFOadkFSQoYVmAMqTmECgYEA9tk8DKycZe7ReCy5\n\
PgxOcEUd6IJubo1T1EiLoP1wmwkpPO/lzCVsbNy3z0iOv2v5aKvKDtfB8Nqi/Tco\n\
eZ8p5aSz6JaYB1OGJylaACfHcYo6j3shTp6zrt7Ny+A9cobAElcBbFNR/kocq/7L\n\
7KgHB5Nw/MMpkW9jHw6PzFXea70CgYEA4PdjXOj5ubsq04aPZMNuZj4YF8jN2UEj\n\
8GDb4g6Yk3nSHk8d+AEK853vw4M8Gx9a/xstEVA3OR+IQMUiwldMpSLjTJPbo+C0\n\
YaO0H4eQvs4+Keb0GpQI9Aoyb47H5RcuL/WGyCjX0a0Q3mBeYK9EaqykoYXYswx+\n\
LrRO2g1ZkasCgYB1A/eWXoDpRTOhDzpk8nqAeBM8dBAbcf1qJVWjZGDr7YTR8Try\n\
8k6asGql5VRd47Zgawcm0BqMrWgUNwCF3vCCfvgV6sV4u5xdlhmm9zRxW0B1Yb4h\n\
QdQVsmLGIa+fScdlKj9vdXPp4oIC+o76yZBDhB5Mg8uBuNa5EzMRfycn1QKBgAJK\n\
IkXAzUHZqM7sUKtK3eXGfgJH9ehfJFl+8t3zHhRqKfxK+9gZUp42HkmUHVl9UTTF\n\
dwj1EwrOylk2aGsn7kzD9SJDYvnhAJT50Ix9mbiHatSMWDGpDgpFzEucXWZ0fPDa\n\
A9wnpkKpC0HoIy3CuuSHC0nljdYMq4AYL7FCb4XvAoGALKigWhqcKC6fnBaI3pbY\n\
hInPc3RWG3kEs0JMZoog698b/L0h5jyKUm6EAOiqbbt4/o9OlY8i2pqr7tAO3479\n\
Og3qyh0GYWKzI1xStm5sv8g6iVLMn2lyE9KDGXMdL43CqS7efCAuVFSQjYaHsEfm\n\
XBRkpp+c1BHH4ErrfbQUQD4=\n\
-----END PRIVATE KEY-----"

#define TESTING_RSA_PUBLIC_KEY "-----BEGIN PUBLIC KEY-----\n\
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2Oyf86AzijQvdoAPOTxx\n\
wk9NmggKfW+Zdj0ESIGnn2oJ1EMhDmv1vdYeENHbd0reG8JKhDcxw7LwgtsyqsOk\n\
mHGzBUfCRUgRcP3tUhhZ6oAwm68W3QzHMqVnlSXEulS506WOrVGDTT+6PgKCsIyR\n\
UGTZghl8HK5CtJVKPcLatW1Fqov33ycD+UvX01W0tQzVtD6apdOgKGmrL9UJjXHv\n\
8M1Ml8dMAyhK8YfB/s4NDVS3MdmkYZXCmo6ujE/e2wfHg1t13I3TAW/jZfvBQmoP\n\
JZPafA2pFmOGyPyI4Vq+exOzK/V2YGvr1qc3Wi6CFf/eh0ifSRlEBhr3skTeZE0E\n\
PwIDAQAB\n\
-----END PUBLIC KEY-----"
#endif