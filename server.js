const express = require('express')
const cors = require('cors')
const bodyParser = require('body-parser')
const methodOverride = require('method-override')
const MongoClient = require('mongodb').MongoClient
const app = express()
const port = 8000

app.use(cors())
app.use(express.json())

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
  .catch((error) => {
    console.error('Database connection error:', error)
    process.exit(1) // Exit the process if the database connection fails
  })

app.post('/api/tracking', (req, res) => {
  const data = req.body
  console.log('Received data:', data)

  db.collection('tracking').insertOne(data, (err, result) => {
    if (err) return res.status(500).send('Error inserting data.')
    res.json({
      message: 'Data received and saved successfully',
      data: result.ops[0],
    })
  })
})

app.get('/api/tracking', (req, res) => {
  db.collection('tracking')
    .find({})
    .toArray((err, result) => {
      if (err) return res.status(500).send('Error fetching data.')
      res.json(result)
    })
})

app.post('/api/location', (req, res) => {
  const data = req.body
  console.log('Received location update:', data)

  db.collection('locationUpdates').insertOne(data, (err, result) => {
    if (err) return res.status(500).send('Error inserting data.')
    res.json({
      message: 'Location update received and saved successfully',
      data: result.ops[0],
    })
  })
})

app.post('/api/battery', (req, res) => {
  const data = req.body
  console.log('Received battery update:', data)

  db.collection('batteryUpdates').insertOne(data, (err, result) => {
    if (err) return res.status(500).send('Error inserting data.')
    res.json({
      message: 'Battery update received and saved successfully',
      data: result.ops[0],
    })
  })
})

app.get('/api/getClientDetails', (req, res) => {
  db.collection('tracking')
    .findOne({}, { sort: { _id: -1 } }) // Sort by `_id` in descending order to get the latest entry
    .then((data) => {
      if (!data) {
        return res.status(404).json({ message: 'No tracking data found' })
      }
      res.json(data)
    })
    .catch((err) => {
      console.error('Error retrieving data:', err)
      res.status(500).send('Error retrieving data.')
    })
})

app.use(methodOverride())
app.use(express.static(__dirname + '/public'))

app.listen(port, () => {
  console.log(`Server listening at http://localhost:${port}`)
})
