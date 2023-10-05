import express, { Express, Request, Response } from "express";
import dotenv from "dotenv";
import fs from "fs"

import cors from "cors";

dotenv.config();

const app: Express = express();
const port = process.env.PORT;

app.use(cors());

app.get('/get_net_bins', (req: Request, res: Response) =>
{
    const build_dir = "../../net/build"
    const files:any = [];

    // Get all the binary files
    const flasher_args:any = JSON.parse(
        fs.readFileSync(`${build_dir}/flasher_args.json`, "utf-8"));
    const flash_files_by_offset = flasher_args["flash_files"];

    // Get keys and values
    console.log(flasher_args);
    console.log(flash_files_by_offset);
    for (let key in flash_files_by_offset)
    {
        const file = flash_files_by_offset[key];
        const name = file.substring(file.indexOf("/")+1, file.indexOf(".bin"))
        const binary = fs.readFileSync(`${build_dir}/${file}`).toJSON().data;

        files.push({
            "name": name,
            "offset": Number(key),
            "binary": binary
        })
    }

    console.log(files);

    //

    // files['bootloader'] = fs.readFileSync(`${build_dir}/bootloader/bootloader.bin`,
    //     null);
    // files['partition_table'] = fs.readFileSync(`${build_dir}/partition_table/partition-table.bin`,
    //     null);
    // files['binary'] = fs.readFileSync(`${build_dir}/net.bin`);


    // console.log(files);

    res.json(files)
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
