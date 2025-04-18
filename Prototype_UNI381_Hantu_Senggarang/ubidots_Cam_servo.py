from machine import Pin, PWM
import time
import network
import urequests

# === KONFIGURASI UBIDOTS ===
TOKEN = 'BBUS-zmx2t4xKQgoSm6Wju4v2suOkJLsSd2'
DEVICE = 'esp32'
COMMAND_VAR = 'command'

# === KONEKSI WIFI ===
SSID = 'Tambelan'
PASSWORD = 'bukacelane'

def connect_wifi():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    if not wlan.isconnected():
        print("Menghubungkan ke WiFi...")
        wlan.connect(SSID, PASSWORD)
        while not wlan.isconnected():
            pass
    print("WiFi Connected:", wlan.ifconfig())

# === BACA NILAI DARI UBIDOTS ===
def get_command_from_ubidots():
    url = f"https://industrial.api.ubidots.com/api/v1.6/devices/{DEVICE}/{COMMAND_VAR}/lv"
    headers = {
        "X-Auth-Token": TOKEN,
        "Content-Type": "application/json"
    }
    try:
        response = urequests.get(url, headers=headers)
        value = float(response.text.strip())
        response.close()
        print("Nilai command dari Ubidots:", value)
        return value
    except Exception as e:
        print("Gagal membaca command:", e)
        return None

# === FUNGSI SET SERVO DENGAN BUZZER ===
buzzer = Pin(5, Pin.OUT)

def move_servo(angle):
    # Hitung duty berdasarkan sudut (cocok untuk SG90)
    min_duty = 30   # 0°
    max_duty = 130  # 180°
    duty = int((angle / 180) * (max_duty - min_duty) + min_duty)

    pwm = PWM(Pin(15), freq=50)
    print(f"Gerakkan servo ke {angle}° (Duty: {duty})")
    
    buzzer.on()
    pwm.duty(duty)
    time.sleep(1.2)  # waktu cukup agar servo selesai
    pwm.deinit()     # matikan sinyal PWM untuk menghindari getaran
    buzzer.off()

# === MAIN LOOP ===
connect_wifi()
last_state = None

while True:
    command_value = get_command_from_ubidots()
    if command_value is not None:
        if command_value == 1 and last_state != 1:
            move_servo(180)
            print("Servo ON (180°)")
            last_state = 1
        elif command_value == 0 and last_state != 0:
            move_servo(0)
            print("Servo OFF (0°)")
            last_state = 0
    time.sleep(5)
