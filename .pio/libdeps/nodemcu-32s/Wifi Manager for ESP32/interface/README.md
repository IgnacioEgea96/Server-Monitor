# Interface

Uses parcel to convert the user interface in `src/index.html` (inline js & css) to a minified version and a gzipped file. After that a node script converts the gzipped file to a 16 columns hexidecimal file that can be used in the Wifi Manager.

```bash
$ yarn && yarn convert
```

### Script outputs

#### `dist/index.hex`
Data to serve to users who sends the `Accept-Encoding: gzip` header.

#### `dist/index.txt`
Data to serve to users who does not accept a gzipped response.

### Move the data
Put the content of the out put files in `Data.cpp` found in `/src`.
