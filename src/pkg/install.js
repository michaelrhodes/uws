const fs = require('fs')
const path = require('path')
const https = require('https')
const pkg = require('./package.json')
const bin = require('./bin')

const src = `https://pkg.mkr.sx/uws/${pkg.version}/${bin}`
const dst = path.join(process.cwd(), bin)

https.get(src, function (res) {
  if (res.statusCode !== 200) throw new Error(`Couldn’t install ${src}`)
  res.pipe(fs.createWriteStream(dst))
})
