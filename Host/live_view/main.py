import json
import pandas as pd
import plotly.express as px
from dash import Dash, dcc, html


import queue
import threading

from collections import deque

import serial

PORT = "/dev/ttyACM2"
BAUDRATE = 115200
TIMEOUT = 1.0
SIZE = 1000



buf = deque(maxlen=SIZE)
q = queue.Queue(maxsize=SIZE)

stop = threading.Event()


import time

def read_serial():
    time.sleep(0.2)
    try:        
        with serial.Serial(PORT, BAUDRATE, timeout=1.0) as s:
            time.sleep(0.1)
            while not stop.is_set():
                l = s.readline().decode("utf-8", errors="ignore").strip()
                if not l:
                    continue
                try:
                    o = json.loads(l)
                except json.JSONDecodeError:
                    continue

                buf.append(o)
                print(o)                
                q.put_nowait(o)


    except Exception as e:
        if not stop.is_set():
            print(e)


def logger():
    time.sleep(0.2)
    with open("serial_log.csv", "a", encoding="utf-8") as f:
        while not stop.is_set() or not q.empty():
            try:
                r = q.get(timeout=0.2)
            except queue.Empty:
                continue
            f.write(json.dumps(r) + "\n")
            f.flush()
            q.task_done()

with open("test_data.json") as f:
    data = json.load(f)

df = pd.DataFrame(data)
if "ts" in df.columns:
    df["time"] = pd.to_datetime(df["ts"], unit="ms")

fig = px.scatter_map(
    df,
    lat="lat",
    lon="lon",
    color="t_c" if "t_c" in df.columns else None,
    hover_data=[c for c in ["time","t_c","rh","seq","src"] if c in df.columns],
    height=900,
    center={"lat": float(df["lat"].mean()), "lon": float(df["lon"].mean())},
    zoom=13,
)

fig.update_traces(marker=dict(size=14))
fig.update_layout(
    map_style="open-street-map",
    margin=dict(l=0, r=0, t=0, b=0),
)

app = Dash(__name__)
app.layout = html.Div([dcc.Graph(id="map", figure=fig)])

if __name__ == "__main__":
    t = threading.Thread(target=read_serial)
    log = threading.Thread(target=logger)

    t.start()
    log.start()

    try:
        app.run(debug=True)  # Dash blokkerer her
    except KeyboardInterrupt:
        pass
    finally:
        stop.set()
        t.join()
        log.join()