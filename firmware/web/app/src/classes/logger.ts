import { reactive } from "vue";

enum LogLevel
{
    Info = 1,
    Warning = 2,
    Error = 4,
    Debug = 8,
};

interface LogEntry
{
    timestamp: Date;
    text: string;
    log_level: LogLevel;
    gui_overwrite: boolean;
};
class Logger
{
    constructor(log_level: LogLevel = LogLevel.Info)
    {
        this.log_level = log_level;
        this.log_buffers = {};
        this.logs = reactive([]);
    }

    SetLogLevel(log_level: LogLevel)
    {
        this.log_level = log_level;
    }

    GetLogLevel(log_level: LogLevel)
    {
        return this.log_level;
    }

    async Info(log: any,
        gui_overwrite: boolean = false)
    {
        this.Log(log, LogLevel.Info, gui_overwrite);
    }

    async Debug(log: any,
        gui_overwrite: boolean = false)
    {
        this.Log(log, LogLevel.Debug, gui_overwrite);
    }

    async Warning(log: any, gui_overwrite: boolean = false)
    {
        this.Log(log, LogLevel.Warning, gui_overwrite);
    }

    async Error(log: any,
        gui_overwrite: boolean = false)
    {
        this.Log(log, LogLevel.Error, gui_overwrite);
    }

    Log(log: any, log_level: LogLevel,
        gui_overwrite: boolean = false)
    {
        // Convert into json string
        let msg = log;
        if (typeof msg !== "string")
            msg = JSON.stringify(msg);

        const entry: LogEntry = {
            timestamp: new Date(),
            text: msg,
            log_level: log_level,
            gui_overwrite: false
        };

        if (!this.log_buffers[log_level])
            this.log_buffers[log_level] = [];

        if (this.logs.length > 0)
        {
            const buff_ref: LogEntry[] = this.log_buffers[log_level] ?? [];
            if (buff_ref.length > 0)
            {
                buff_ref[buff_ref.length - 1].gui_overwrite = gui_overwrite;
            }

            if (this.logs[this.logs.length - 1].log_level == log_level)
            {
                this.logs[this.logs.length - 1].gui_overwrite = gui_overwrite;
            }
        }

        this.log_buffers[log_level]?.push(entry);
        this.logs.push(entry);

        if ((log_level & this.log_level) > 0)
        {
            const lvl_str = Logger.LogLevelToString(log_level);
            const date = `${entry.timestamp.getHours()}:` +
                `${entry.timestamp.getMinutes()}:` +
                `${entry.timestamp.getSeconds()}`;
            const colour = this.GetColour(log_level);
            console.log(`%c[${lvl_str}] ${date} - ${msg}`, colour);
        }
    }

    GetLogBuffers()
    {
        return this.log_buffers;
    }

    GetLogs()
    {
        return this.logs;
    }

    GetColour(log_level: LogLevel)
    {
        switch (log_level)
        {
            case LogLevel.Warning:
                return 'color: yellow';
            case LogLevel.Error:
                return 'color: red';
            case LogLevel.Debug:
                return 'color: magenta';
            default:
                return 'color: white';
        }
    }

    static LogTime(entry: LogEntry)
    {
        const hours = entry.timestamp.getHours();
        const minutes = entry.timestamp.getMinutes();
        const seconds = entry.timestamp.getSeconds();

        return `${hours}:${minutes}:${seconds}`;
    }

    static LogLevelToString(log_level: LogLevel)
    {
        switch (log_level)
        {
            case LogLevel.Info:
                return 'INFO ';
            case LogLevel.Warning:
                return 'WARN ';
            case LogLevel.Error:
                return 'ERROR';
            case LogLevel.Debug:
                return 'DEBUG';
            default:
                return 'UNKNOWN';
        }
    }

    log_level: LogLevel
    log_buffers: { [key in LogLevel]?: LogEntry[] };
    logs: LogEntry[];

};

const logger = new Logger();
export default logger;

const LogLevelToString = Logger.LogLevelToString;
const LogTime = Logger.LogTime;
export { LogLevel, LogLevelToString, LogTime };
export type { LogEntry };