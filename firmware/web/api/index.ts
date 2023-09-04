import express, { Express, Request, Response } from "express";
import dotenv from "dotenv";
import fs from "fs"

import cors from "cors";

dotenv.config();

const app: Express = express();
const port = process.env.PORT;

app.use(cors());

app.get('/', (req: Request, res: Response) => {
    let bin = fs.readFileSync("./ui.bin", null);

    console.log(bin[3])

    res.json(bin)
});

app.get('/get_ui_bin', (req: Request, res: Response) => {

});


app.listen(port, () => {
    console.log(`⚡️[server]: Server is running at http://localhost:${port}`);
});
