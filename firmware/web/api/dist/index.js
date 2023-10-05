"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const express_1 = __importDefault(require("express"));
const dotenv_1 = __importDefault(require("dotenv"));
const fs_1 = __importDefault(require("fs"));
const cors_1 = __importDefault(require("cors"));
dotenv_1.default.config();
const app = (0, express_1.default)();
const port = process.env.PORT;
app.use((0, cors_1.default)());
app.get('/get_net_bins', (req, res) => {
    const build_dir = "../../net/build";
    const files = [];
    // Get all the binary files
    const flasher_args = JSON.parse(fs_1.default.readFileSync(`${build_dir}/flasher_args.json`, "utf-8"));
    const flash_files_by_offset = flasher_args["flash_files"];
    // Get keys and values
    console.log(flasher_args);
    console.log(flash_files_by_offset);
    for (let key in flash_files_by_offset) {
        const file = flash_files_by_offset[key];
        const name = file.substring(file.indexOf("/") + 1, file.indexOf(".bin"));
        const binary = fs_1.default.readFileSync(`${build_dir}/${file}`).toJSON().data;
        files.push({
            "name": name,
            "offset": Number(key),
            "binary": binary
        });
    }
    console.log(files);
    //
    // files['bootloader'] = fs.readFileSync(`${build_dir}/bootloader/bootloader.bin`,
    //     null);
    // files['partition_table'] = fs.readFileSync(`${build_dir}/partition_table/partition-table.bin`,
    //     null);
    // files['binary'] = fs.readFileSync(`${build_dir}/net.bin`);
    // console.log(files);
    res.json(files);
});
app.get('/get_ui_bins', (req, res) => {
    let bin = fs_1.default.readFileSync("../../ui/build/ui.bin", null);
    res.json(bin.toJSON().data);
});
app.listen(port, () => {
    console.log(`⚡️[server]: Server is running at http://localhost:${port}`);
});
