var hostname = process.env.HOSTNAME || 'localhost'
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
    db = client.db('trackingData')
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

app.use(methodOverride())
app.use(bodyParser())
app.use(express.static(__dirname + '/public'))
app.use(errorHandler())

console.log('Simple static server listening at http://' + hostname + ':' + port)
app.listen(port)
