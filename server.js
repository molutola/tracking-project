// const express = require('express')
// const cors = require('cors')
// const app = express()
// const port = 8000

// app.use(cors()) // Enable CORS for all routes
// app.use(express.json())

// let MongoClient = require('mongodb').MongoClient
// const connectionString = 'mongodb://localhost:27017'
// let db

// // Connect to MongoDB
// MongoClient.connect(connectionString, {
//   useNewUrlParser: true,
//   useUnifiedTopology: true,
// })
//   .then((client) => {
//     db = client.db('trackingData') // Use the trackingData database
//     console.log('Connected to Database')
//   })
//   .catch((error) => console.error(error))

// // Middleware to parse JSON and URL-encoded data
// app.use(express.json())
// app.use(cors())
// app.use(express.urlencoded({ extended: true }))

// app.get('/', function (req, res) {
//   res.redirect('/index.html')
// })

// // Route to handle tracking data submission
// app.post('/api/tracking', (req, res) => {
//   const data = req.body
//   console.log('Received data:', data)

//   // Save data to MongoDB
//   db.collection('tracking').insertOne(data, (err, result) => {
//     if (err) return res.status(500).send('Error inserting data.')
//     res.json({
//       message: 'Data received and saved successfully',
//       data: result.ops[0],
//     })
//   })
// })
// app.listen(port, () => {
//   console.log(`Server running at http://localhost:${port}`)
// })

const express = require('express')
const cors = require('cors')
const app = express()
const port = 8000

app.use(cors()) // Enable CORS for all routes
app.use(express.json())

let MongoClient = require('mongodb').MongoClient
const connectionString = 'mongodb://localhost:27017'
let db

MongoClient.connect(connectionString, {
  useNewUrlParser: true,
  useUnifiedTopology: true,
})
  .then((client) => {
    db = client.db('trackingData') // Use the trackingData database
    console.log('Connected to Database')
  })
  .catch((error) => console.error(error))

app.post('/api/tracking', (req, res) => {
  const data = req.body
  console.log('Received data:', data)
  res.json({ message: 'Data received successfully', data })

  // Save data to MongoDB
  db.collection('tracking').insertOne(data, (err, result) => {
    if (err) return res.status(500).send('Error inserting data.')
    res.json({
      message: 'Data received and saved successfully',
      data: result.ops[0],
    })
  })
})

app.listen(port, () => {
  console.log(`Server running at http://localhost:${port}`)
})
