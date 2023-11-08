"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const express_1 = __importDefault(require("express"));
const dotenv_1 = __importDefault(require("dotenv"));
const fs_1 = __importDefault(require("fs"));
const crypto_1 = __importDefault(require("crypto"));
const cors_1 = __importDefault(require("cors"));
dotenv_1.default.config();
const app = (0, express_1.default)();
const port = process.env.PORT;
function GetFiles() {
    const build_dir = "../../net/build";
    const files = [];
    // Get all the binary files
    try {
        const flasher_args = JSON.parse(fs_1.default.readFileSync(`${build_dir}/flasher_args.json`, "utf-8"));
        const flash_files_by_offset = flasher_args["flash_files"];
        // Get keys and values
        for (let key in flash_files_by_offset) {
            const file = flash_files_by_offset[key];
            const name = file.substring(file.indexOf("/") + 1, file.indexOf(".bin"));
            const binary = fs_1.default.readFileSync(`${build_dir}/${file}`).toJSON().data;
            // Convert int array in 8 bit array
            let t = Uint8Array.from(binary);
            const hash = crypto_1.default.createHash('md5').update(t).digest("hex").toString();
            files.push({
                "name": name,
                "offset": Number(key),
                "binary": binary,
                "md5": hash,
            });
        }
    }
    catch (exception) {
        console.log(exception);
        console.log("No net binaries found");
    }
    return files;
}
app.use((0, cors_1.default)());
app.get('/get_net_bins', (req, res) => {
    const files = GetFiles();
    res.json(files);
});
app.get('/get_ui_bins', (req, res) => {
    let bin = fs_1.default.readFileSync("../../ui/build/ui.bin", null);
    res.json(bin.toJSON().data);
});
app.listen(port, () => {
    console.log(`⚡️[server]: Server is running at http://localhost:${port}`);
});
