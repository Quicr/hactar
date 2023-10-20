import express, { Express, Request, Response } from "express";
import dotenv from "dotenv";
import fs from "fs"

import crypto from "crypto";
import cors from "cors";

dotenv.config();

const app: Express = express();
const port = process.env.PORT;

function GetFiles()
{
    const build_dir = "../../net/build"
    const files:any = [];

    // Get all the binary files
    const flasher_args:any = JSON.parse(
        fs.readFileSync(`${build_dir}/flasher_args.json`, "utf-8"));
    const flash_files_by_offset = flasher_args["flash_files"];

    // Get keys and values
    for (let key in flash_files_by_offset)
    {
        const file = flash_files_by_offset[key];
        const name = file.substring(file.indexOf("/")+1, file.indexOf(".bin"))
        const binary = fs.readFileSync(`${build_dir}/${file}`).toJSON().data;

        // Convert int array in 8 bit array
        let t:any = Uint8Array.from(binary);
        const hash = crypto.createHash('md5').update(t).digest("hex").toString();


        files.push({
            "name": name,
            "offset": Number(key),
            "binary": binary,
            "md5": hash,
        })
    }

    return files;
}

GetFiles();

app.use(cors());

app.all('/auth', (req: Request, res: Response) => 
{
    // TODO authentication
});

app.get('/get_net_bins', (req: Request, res: Response) =>
{
    const files = GetFiles();
    res.json(files);
});

app.get('/get_ui_bins', (req: Request, res: Response) =>
{
    let bin = fs.readFileSync("../../ui/build/ui.bin", null);

    res.json(bin.toJSON().data);
});


app.listen(port, () =>
{
    console.log(`⚡️[server]: Server is running at http://localhost:${port}`);
});
