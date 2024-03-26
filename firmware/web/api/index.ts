import express, { Express, Request, Response } from "express";
import dotenv from "dotenv";
import fs from "fs"

import crypto from "crypto";
import cors from "cors";

dotenv.config();

const app: Express = express();
const port = process.env.PORT;

function GetEspFiles()
{
    const build_dir = "../../net/build"
    const files: any = [];

    // Get all the binary files
    try
    {
        const flasher_args: any = JSON.parse(
            fs.readFileSync(`${build_dir}/flasher_args.json`, "utf-8"));
        const flash_files_by_offset = flasher_args["flash_files"];

        // Get keys and values
        for (let key in flash_files_by_offset)
        {
            const file = flash_files_by_offset[key];
            const name = file.substring(file.indexOf("/") + 1, file.indexOf(".bin"))
            const binary = fs.readFileSync(`${build_dir}/${file}`).toJSON().data;

            // Convert int array in 8 bit array
            let t: any = Uint8Array.from(binary);
            const hash = crypto.createHash('md5').update(t).digest("hex").toString();


            files.push({
                "name": name,
                "offset": Number(key),
                "binary": binary,
                "md5": hash,
            })
        }
    }
    catch(exception)
    {
        console.log(exception);
        console.log("No net binaries found");
    }

    return files;
}

app.use(cors());

app.all('/auth', (req: Request, res: Response) =>
{
    // TODO authentication
});

// TODO expand to use github repos
app.get('/firmware', (req: Request, res: Response) =>
{
    console.log(req.query)

    // TODO use a db connection to query the location of the bin,
    // given the version

    // but for now lets just if statement and grab latest
    if (req.query["bin"] == "mgmt")
    {
        const bin = fs.readFileSync("../../mgmt/build/mgmt.bin", null);
        res.json(bin.toJSON().data);
    }
    else if (req.query["bin"] == "ui")
    {
        const bin = fs.readFileSync("../../ui/build/ui.bin", null);
        res.json(bin.toJSON().data);
    }
    else if (req.query["bin"] == "net")
    {
        const bin = GetEspFiles();
        res.json(bin);
    }
});

app.get('/stm_configuration', (req: Request, res: Response) =>
{
    // TODO error if the configuration exists
    let device_id:any = req.query.uid;
    let configs = JSON.parse(fs.readFileSync("stm32_configurations.json", "utf8"));
    let config = configs[device_id]
    console.log(`Returning configuration for ${config['name']}`);
    res.json(config);

});

app.listen(port, () =>
{
    console.log(`⚡️[server]: Server is running at http://localhost:${port}`);
});
