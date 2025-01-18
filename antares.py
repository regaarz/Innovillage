import streamlit as st
import requests
import json
import time

# Konfigurasi Antares
ACCESS_KEY = "1ae01381c63e2d54:93a2cacfa9a654c9"
PROJECT_NAME = "Hydrats"
DEVICE_NAME = "monitoringBendungan"
BASE_URL = "https://platform.antares.id:8443/~/antares-cse/antares-id"

# Fungsi untuk mendapatkan data terbaru dari Antares
def get_antares_data():
    url = f"{BASE_URL}/{PROJECT_NAME}/{DEVICE_NAME}/la"
    headers = {
        "X-M2M-Origin": ACCESS_KEY,
        "Content-Type": "application/json;ty=4"
    }
    response = requests.get(url, headers=headers)
    
    if response.status_code == 200:
        data = response.json()
        # Parsing JSON response untuk mendapatkan data turbidity
        content = json.loads(data["m2m:cin"]["con"])
        return content.get("Kekeruhan", "Data not available")
    else:
        return f"Error: {response.status_code}"

# Streamlit UI
st.title("Real-Time Turbidity Monitoring")
st.write("Data sensor turbidity yang dikirim dari perangkat ESP32 ke Antares.")

# Interval untuk auto-refresh
refresh_interval = 5  # Detik

# Tampilkan data
turbidity_value = get_antares_data()
if isinstance(turbidity_value, str) and "Error" in turbidity_value:
    st.error(turbidity_value)
else:
    st.metric(label="Turbidity (NTU)", value=f"{turbidity_value} NTU")

# Auto-refresh halaman
time.sleep(refresh_interval)
st.experimental_rerun()
