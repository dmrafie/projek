import asyncio
import serial
import time
import winrt.windows.media.control as wmc

# Change to your actual COM port
PORT = "COM3" 
ser = serial.Serial(PORT, 115200, timeout=0.1)

async def get_media_info():
    manager = await wmc.GlobalSystemMediaTransportControlsSessionManager.request_async()
    session = manager.get_current_session()
    
    if session:
        props = await session.try_get_media_properties_async()
        info = session.get_playback_info()
        timeline = session.get_timeline_properties()
        
        # 1. Determine Status
        is_playing = info.playback_status == wmc.GlobalSystemMediaTransportControlsSessionPlaybackStatus.PLAYING
        status = "playing" if is_playing else "paused"
        
        # 2. Get Metadata
        title = props.title if props.title else "Unknown Title"
        artist = props.artist if props.artist else "Unknown Artist"
        
        # 3. Calculate REAL Progress
        # Windows gives us a position at a specific 'LastUpdatedTime'
        pos_at_update = timeline.position.total_seconds()
        
        if is_playing:
            # Calculate how many seconds passed since Windows last updated the timeline
            # We use the system clock to find the difference
            time_since_update = (time.time() - timeline.last_updated_time.timestamp())
            actual_pos = int(pos_at_update + time_since_update)
        else:
            actual_pos = int(pos_at_update)

        dur = int(timeline.end_time.total_seconds())
        
        # Prevent showing progress > duration
        if actual_pos > dur: actual_pos = dur
        
        return f"{status}|{title}|{artist}|{actual_pos}|{dur}"
    
    return "paused|No Media|Unknown|0|0"

async def main():
    print(f"Syncing Windows Media to {PORT}...")
    while True:
        try:
            # Update data
            data = await get_media_info()
            ser.write((data + "\n").encode('utf-8'))
            
            # Listen for ESP32-C3 Button Commands
            if ser.in_waiting > 0:
                line = ser.readline().decode().strip()
                manager = await wmc.GlobalSystemMediaTransportControlsSessionManager.request_async()
                s = manager.get_current_session()
                if s:
                    if line == "PLAYPAUSE": await s.try_toggle_play_pause_async()
                    elif line == "NEXT": await s.try_skip_next_async()
                    elif line == "PREV": await s.try_skip_previous_async()
        except Exception as e:
            print(f"Error: {e}")
            
        await asyncio.sleep(1)

if __name__ == "__main__":
    asyncio.run(main())