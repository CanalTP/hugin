# Json settings file for elastic search


Note:
It's not possible to put comments in json, so the comments on the conf files are here

## index_settings.json

TODO we need to explain for each fields the reason for their type and index


### admin

 * shape/admin shape
for the moment we use a default quadtree with 1m precision. We need to benchmark if other indexes are better

 * coord
the format is compressed to reduce the memory foot print

with https://www.elastic.co/guide/en/elasticsearch/reference/current/mapping-geo-point-type.html 
we see that 1m is the same as 1cm, so we use 1cm


