import serial
import time
import os

# Change to your actual port (find it with: ls /dev/cu.usbmodem*)
port = "/dev/cu.usbmodem14201" 
ser = serial.Serial(port, 115200, timeout=0.1)

def get_music_info():
    # AppleScript to get detailed info
    script = '''
    tell application "Music"
        if it is running then
            set pState to player state as text
            set pName to name of current track
            set pArtist to artist of current track
            set pPos to player position
            set pDur to duration of current track
            return pState & "|" & pName & "|" & pArtist & "|" & pPos & "|" & pDur
        else
            return "stopped|None|None|0|0"
        end if
    end tell
    '''
    try:
        return os.popen(f"osascript -e '{script}'").read().strip()
    except:
        return "error|None|None|0|0"

def send_command(cmd):
    script = f'tell application "Music" to {cmd}'
    os.system(f"osascript -e '{script}'")

while True:
    # 1. Send Update to ESP32
    info = get_music_info()
    ser.write((info + "\n").encode())

    # 2. Check for Button Commands from ESP32
    if ser.in_waiting > 0:
        btn_cmd = ser.readline().decode().strip()
        if btn_cmd == "PLAYPAUSE": send_command("playpause")
        elif btn_cmd == "NEXT": send_command("next track")
        elif btn_cmd == "PREV": send_command("previous track")

    time.sleep(1) # Refresh rate