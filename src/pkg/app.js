module.exports = app;

const uws = require('./main');

function app (opts) {
  return opts ? uws.SSLApp(opts) : uws.App();
}
