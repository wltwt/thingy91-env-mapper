import json
import pandas as pd
import plotly.express as px
from dash import Dash, dcc, html
from dash.dependencies import Input, Output

import queue
import threading
from collections import deque
import serial
import time

PORT = "/dev/ttyACM5"
BAUDRATE = 115200
TIMEOUT = 1.0
SIZE = 1000

buf = deque(maxlen=SIZE)
q = queue.Queue(maxsize=SIZE)
stop = threading.Event()
buf_lock = threading.Lock()

# normaliser data
def normalize(o):
    out = o
    if "lat_e7" in out and "lat" not in out:
        out["lat"] = out["lat_e7"] / 1e7
    if "lon_e7" in out and "lon" not in out:
        out["lon"] = out["lon_e7"] / 1e7
        
    if "t_c_e2" in out and "t_c" not in out:
        out["t_c"] = out["t_c_e2"] / 100.0
    if "rh_e2" in out and "rh" not in out:
        out["rh"] = out["rh_e2"] / 100.0

    return out

# serial leser-tråd
def read_serial():
    time.sleep(0.2)
    try:
        with serial.Serial(PORT, BAUDRATE, timeout=TIMEOUT) as s:
            time.sleep(0.1)
            while not stop.is_set():
                l = s.readline().decode("utf-8", errors="ignore").strip()
                if not l:
                    continue
                if not l.startswith("{"):
                    continue

                try:
                    o = normalize(json.loads(l))
                except json.JSONDecodeError:
                    continue

                with buf_lock:
                    buf.append(o)

                print(o)
                try:
                    q.put_nowait(o)
                except queue.Full:
                    pass
    except Exception as e:
        if not stop.is_set():
            print(e)

# logger-tråd
def logger():
    time.sleep(0.2)
    with open("serial_log.jsonl", "a", encoding="utf-8") as f:
        while not stop.is_set() or not q.empty():
            try:
                r = q.get(timeout=0.2)
            except queue.Empty:
                continue
            f.write(json.dumps(r, ensure_ascii=False) + "\n")
            f.flush()
            q.task_done()


FALLBACK_LOCATION = {"lat": 63.4305, "lon": 10.3951} # fallback trondheim
LAST_ZOOM = 15

# oppdater plotly graf
def make_fig(df: pd.DataFrame):
    global FALLBACK_LOCATION, LAST_ZOOM

    if "ts" in df.columns:
        df["time"] = pd.to_datetime(df["ts"], unit="ms", errors="coerce")
    if "lat" in df.columns and "lon" in df.columns:
        df_valid = df[(df["lat"] != 0) & (df["lon"] != 0)].copy()
    else:
        df_valid = pd.DataFrame()


    if df_valid.empty:
        fig = px.scatter_map(
            lat=[],
            lon=[],
            height=900,
            center=FALLBACK_LOCATION,
            zoom=LAST_ZOOM,
        )
        fig.update_layout(map_style="open-street-map", margin=dict(l=0, r=0, t=0, b=0))
        return fig

    FALLBACK_LOCATION = {"lat": float(df_valid["lat"].mean()), "lon": float(df_valid["lon"].mean())}

    fig = px.scatter_map(
        df_valid,
        lat="lat",
        lon="lon",
        color="t_c" if "t_c" in df_valid.columns else None,
        hover_data=[c for c in ["time", "t_c", "rh", "seq", "src", "rssi"] if c in df_valid.columns],
        height=900,
        center=FALLBACK_LOCATION,
        zoom=LAST_ZOOM,
    )
    fig.update_traces(marker=dict(size=14))
    last = df.iloc[-1]
    fig.update_layout(
    mapbox_center={
        "lat": float(last["lat"]),
        "lon": float(last["lon"]),
    },
    mapbox_zoom=17,
)
    return fig



app = Dash(__name__)
app.layout = html.Div([
    dcc.Graph(id="map"),
    dcc.Interval(id="tick", interval=1000, n_intervals=0),
])

@app.callback(Output("map", "figure"), Input("tick", "n_intervals"))
def update_map(_):
    with buf_lock:
        data = list(buf)
    df = pd.DataFrame(data)
    return make_fig(df)


if __name__ == "__main__":
    t = threading.Thread(target=read_serial, daemon=True)
    log = threading.Thread(target=logger, daemon=True)

    t.start()
    log.start()

    try:
        app.run(debug=True)
    finally:
        stop.set()
        t.join(timeout=1.0)
        log.join(timeout=1.0)
