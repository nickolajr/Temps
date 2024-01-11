var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var ws;
// Chart variables
var temperatureData = [];
var temperatureChart;

// Init web socket when the page loads
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
    initTemperatureChart();
}
function initTemperatureChart() {
    // If temperatureChart already exists, destroy it
    if (temperatureChart) {
        temperatureChart.destroy();
    }

    temperatureChart = new Chart(document.getElementById('temperatureGraph').getContext('2d'), {
        type: 'line',
        data: {
            datasets: [{
                data: temperatureData,
                // other chart options...
            }]
        },
        options: {
            scales: {
                x: {
                    ticks: {
                        display: false // This will hide the labels on the x-axis
                    }
                },
                y: {
                    beginAtZero: false, // start the y-axis at the lowest value, not 0
                    ticks: {
                        
                        // dynamically adjust the y-axis to the data
                        min: Math.min(...temperatureData.map(data => data.y)),
                        max: Math.max(...temperatureData.map(data => data.y))
                    }
                }
            },
            // other chart options...
        }
    });
}
function getReadings() {
    websocket.send("getReadings");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

// When websocket is established, call the getReadings() function
function onOpen(event) {
    console.log('Connection opened');
    getReadings();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

// Function that receives the message from the ESP32 with the readings
function onMessage(event) {
    var lines = event.data.trim().split('\n'); // Split the data into separate lines

    for (var i = 0; i < lines.length; i++) {
        if (lines[i]) { // Ignore empty lines
            var jsonString = lines[i].trim();
            var data = JSON.parse(jsonString);

            var temperature = data.temperature;
            var timestamp = new Date(data.timestamp * 1000); // Convert to milliseconds

            // Add the new data point to your temperatureData array
            temperatureData.push({ x: timestamp, y: temperature });

            // Update the graph
            updateTemperatureGraph();

            // Update the temperature display
            document.getElementById('temperature').textContent = temperature;
        }
    }
}




// Function to initialize the temperature graph
function initGraph() {
    var ctx = document.getElementById('temperatureGraph').getContext('2d');
     temperatureChart = new Chart(ctx, {
        type: 'line',
        data: {
            datasets: [{
                label: 'Temperature',
                data: temperatureData,
                borderColor: 'rgba(75, 192, 192, 1)',
                borderWidth: 2,
                fill: false
            }]
        },
        options: {
            scales: {
                x: {
                    type: 'time',  // Use 'time' type for the x-axis
                    time: {
                        unit: 'second'  // Display units in seconds
                    }
                },
                y: {
                    max: 40,
                    min: 0
                }
            }
        }
    });
}


// Function to update the graph with new temperature data
function updateTemperatureGraph(temperature) {
    var timestamp = new Date();
    temperatureData.push({ x: timestamp, y: parseFloat(temperature) });

    // Limit the number of data points displayed to avoid clutter
    if (temperatureData.length > 20) {
        temperatureData.shift();
    }

    // Update the labels array with timestamps
    temperatureChart.data.labels = temperatureData.map(point => point.x);
    
    temperatureChart.update();
}


// Call initGraph() to initialize the graph when the page loads
window.addEventListener('load', initGraph);

function downloadCSV() {
    // Convert temperatureData to CSV
    var csv = 'Time,Temperature\n';
    temperatureData.forEach(function(row) {
        csv += row.x + ',' + row.y + '\n';
    });

    // Create a downloadable link
    var csvBlob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
    var csvUrl = URL.createObjectURL(csvBlob);
    var hiddenElement = document.createElement('a');
    hiddenElement.href = csvUrl;
    hiddenElement.target = '_blank';
    hiddenElement.download = 'temperatureData.csv';
    hiddenElement.click();
}

function clearCSV() {
    fetch('/clearcsv')
        .then(response => response.text())
        .then(data => console.log(data));
}

function clearGraph() {
    // Clear the temperatureData array
    temperatureData = [];

    // Initialize the chart with the cleared data
    initTemperatureChart();

    // Update the chart
    temperatureChart.update();
}
