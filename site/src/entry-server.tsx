import { createHandler, StartServer } from "@solidjs/start/server";

const themeBootstrap =
  'try{var t=localStorage.getItem("auris-theme");if(t==="light"||t==="dark")document.documentElement.dataset.theme=t}catch(e){}';

export default createHandler(() => (
  <StartServer
    document={({ assets, children, scripts }) => (
      <html lang="en" data-theme="dark">
        <head>
          <meta charset="utf-8" />
          <meta name="viewport" content="width=device-width, initial-scale=1" />
          <link rel="icon" type="image/svg+xml" href="/favicon.svg" />
          <script innerHTML={themeBootstrap} />
          {assets}
        </head>
        <body>
          {children}
          {scripts}
        </body>
      </html>
    )}
  />
));
