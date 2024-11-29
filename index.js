const axios = require('axios')

const clientDetailsUrl = 'http://localhost:8000/api/getClientDetails'

async function getClientDetailsAndLocation() {
  try {
    const response = await axios.get(clientDetailsUrl)
    const data = response.data

    console.log('Server response:', data)

    const clientId = data.clientId
    const destLat = data.destLat
    const destLng = data.destLng
    const phoneNumber = data.phoneNumber

    console.log('Client ID:', clientId)
    console.log('Destination Latitude:', destLat)
    console.log('Destination Longitude:', destLng)
    console.log('Phone Number:', phoneNumber)
  } catch (error) {
    console.error('Error on HTTP request:', error.message)
  }
}

getClientDetailsAndLocation()
