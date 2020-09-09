var mouse = require('../')

var m1 = mouse()

m1.on('move', function (x, y) {
  console.log('move_1', x, y)
})

setTimeout(function () {
  m1.destroy()
  console.log('destroy_1')

  var m2 = mouse()

  m2.on('move', function (x, y) {
    console.log('move_2', x, y)
  })

  setTimeout(function () {
    m2.destroy()
    console.log('destroy_2')
  }, 1000)
}, 1000)
