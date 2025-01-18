import streamlit as st
from streamlit_option_menu import option_menu
import paho.mqtt.client as mqtt
import time
import os

# MQTT Settings
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPICS = {
    "tds": "esp32/hydrats/innovillage/tds",
    "ph": "esp32/hydrats/innovillage/ph",
    "water_temperature": "esp32/hydrats/innovillage/water_temperature",
    "turbidity": "esp32/hydrats/innovillage/turbidity",
    "water_flow": "esp32/hydrats/innovillage/water_flow",
    "distance": "esp32/hydrats/innovillage/distance",
    "water_quality": "esp32/hydrats/innovillage/water_quality"
}

# Global variables to store sensor data
sensor_data = {key: "N/A" for key in MQTT_TOPICS}

# Include CSS for styling (optional)
def include_css(file_name):
    if os.path.exists(file_name):
        with open(file_name) as f:
            st.markdown(f"<style>{f.read()}</style>", unsafe_allow_html=True)
    else:
        st.error(f"CSS file not found: {file_name}")

include_css("styles.css")

# MQTT Callback Functions
def on_message(client, userdata, msg):
    global sensor_data
    for key, topic in MQTT_TOPICS.items():
        if msg.topic == topic:
            sensor_data[key] = msg.payload.decode("utf-8")

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        for topic in MQTT_TOPICS.values():
            client.subscribe(topic)
        st.write("Connected to MQTT broker and subscribed to topics.")
    else:
        st.error(f"Failed to connect, return code {rc}")

# Initialize MQTT Client
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

try:
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_start()
except Exception as e:
    st.error(f"Failed to connect to MQTT broker: {e}")

# Navbar setup
menu_selection = option_menu(
    menu_title=None,  # Leave blank to hide title
    options=["Home", "Monitoring", "About Us"],  # Add more options as needed
    icons=["house", "clipboard-data", "info-circle"],  # Add corresponding icons
    default_index=0,
    orientation="horizontal"
)

# Navbar logic
if menu_selection == "Home":
    st.title("Welcome to the Monitoring System")
    st.write("Explore real-time sensor monitoring and manage your system effectively.")
elif menu_selection == "Monitoring":
    st.title("📊 Real-Time Sensor Data Monitoring")
    st.write("Monitoring multiple sensor values in real-time.")
    data_placeholder = st.empty()

    while True:
        with data_placeholder.container():
            st.markdown(f"""
            <div class="sensor-box">
                <div class="sensor-title">TDS Value</div>
                <div class="sensor-value">{sensor_data['tds']} ppm</div>
            </div>
            <div class="sensor-box">
                <div class="sensor-title">pH Value</div>
                <div class="sensor-value">{sensor_data['ph']}</div>
            </div>
            <div class="sensor-box">
                <div class="sensor-title">Water Temperature</div>
                <div class="sensor-value">{sensor_data['water_temperature']} °C</div>
            </div>
            <div class="sensor-box">
                <div class="sensor-title">Turbidity</div>
                <div class="sensor-value">{sensor_data['turbidity']} NTU</div>
            </div>
            <div class="sensor-box">
                <div class="sensor-title">Water Flow Rate</div>
                <div class="sensor-value">{sensor_data['water_flow']} L/min</div>
            </div>
            <div class="sensor-box">
                <div class="sensor-title">Distance (Ultrasonic)</div>
                <div class="sensor-value">{sensor_data['distance']} cm</div>
            </div>
            <div class="sensor-box">
                <div class="sensor-title">Water Quality</div>
                <div class="sensor-value">{sensor_data['water_quality']}</div>
            </div>
            """, unsafe_allow_html=True)
        time.sleep(1)
elif menu_selection == "About Us":
    st.title("About Us")
    st.write("This is the about us section where you can provide details about your project or team.")
