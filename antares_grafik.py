import streamlit as st
import requests
import json
import matplotlib.pyplot as plt
from datetime import datetime
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
        return content.get("Kekeruhan", None)
    else:
        st.error(f"Error fetching data: {response.status_code}")
        return None

# Streamlit UI
st.title("Real-Time Turbidity Monitoring")
st.write("Grafik real-time dari data turbidity yang diambil dari Antares.")

# Inisialisasi data
timestamps = []
turbidity_values = []

# Placeholder untuk grafik
chart_placeholder = st.empty()

# Loop untuk memperbarui data dan grafik
while True:
    turbidity_value = get_antares_data()

    if turbidity_value is not None:
        # Tambahkan data baru
        current_time = datetime.now().strftime("%H:%M:%S")
        timestamps.append(current_time)
        turbidity_values.append(float(turbidity_value))

        # Batasi data menjadi 10 titik terakhir
        if len(timestamps) > 10:
            timestamps = timestamps[-10:]
            turbidity_values = turbidity_values[-10:]

        # Buat grafik
        fig, ax = plt.subplots()
        ax.plot(timestamps, turbidity_values, marker="o", linestyle="-")
        ax.set_title("Turbidity Over Time (Real-Time)")
        ax.set_xlabel("Time")
        ax.set_ylabel("Turbidity (NTU)")
        ax.set_xticks(timestamps)
        ax.set_xticklabels(timestamps, rotation=45, ha="right")
        ax.grid(True)

        # Tampilkan grafik di Streamlit
        chart_placeholder.pyplot(fig)

    else:
        st.warning("Unable to fetch data from Antares. Please check the configuration.")

    time.sleep(5)  # Tunggu 5 detik sebelum update berikutnya
