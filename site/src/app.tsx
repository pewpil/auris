import { Meta, MetaProvider, Title } from "@solidjs/meta";
import { Router } from "@solidjs/router";
import { FileRoutes } from "@solidjs/start/router";
import { Suspense } from "solid-js";
import "./app.css";

export default function App() {
  return (
    <Router
      root={(props) => (
        <MetaProvider>
          <Title>Auris — hear where things are</Title>
          <Meta
            name="description"
            content="Auris turns a room into a 3D soundscape: a camera array tracks objects and your head in 6DoF, and a headset anchors a distinct sound to every object's real position."
          />
          <Suspense>{props.children}</Suspense>
        </MetaProvider>
      )}
    >
      <FileRoutes />
    </Router>
  );
}
