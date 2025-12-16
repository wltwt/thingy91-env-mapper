import json
import pandas as pd
import plotly.express as px
from dash import Dash, dcc, html

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
    app.run(debug=True)
