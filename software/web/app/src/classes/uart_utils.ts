export let ACK: number = 0x79;
export let READY: number = 0x80;
export let NACK: number = 0x1F;
export let NO_REPLY: number = -1;


export function FromByteArray(array: number[], endian: string = "big")
{
    if (array.length > 4)
        throw "JS can't handle more than like 6 byte integers because \
                'precision'";

    endian = endian.toLowerCase();

    if (endian != "big" && endian != "little")
        throw `Passed in endian=${endian} format is unknown.
                Use 'big' or 'little'`

    if (endian == "little")
        array.reverse();

    let value = 0
    for (let i = 0 ; i < array.length; ++i)
    {
        value = value << 8;
        value += array[i];
    }

    return value;
}

export function ToByteArray(array: number[], bytes_per_element: number,
    endian: string = "big")
{
    if (bytes_per_element > 4)
        throw "JS can't handle more than like 6 byte integers because \
                precision";

    endian = endian.toLowerCase();

    if (endian != "big" && endian != "little")
        throw `Passed in endian=${endian} format is unknown.
            Use 'big' or 'little'`

    let tmp_bytes = []
    let array_bytes = []
    let byte_count = 0;
    let value = 0;
    let mask = 0;
    let bits = 0;
    let byte = 0;

    for (let i = 0; i < array.length; ++i)
    {
        byte_count = bytes_per_element;
        value = array[i];
        tmp_bytes = []

        mask = (0x100 ** (bytes_per_element)) - 1;
        while (byte_count > 0)
        {
            // Get the number of bits to shift by for this byte
            bits = (byte_count - 1) * 8;

            // Get the first order bytes and turn them into bits
            byte = (value & mask) >>> bits;

            // Shift the mask down
            mask = mask >>> 8;

            // Mask out the new converted bits
            value = value & mask;

            // Push the byte into the array
            tmp_bytes.push(byte);

            // Decrement how many bytes we are calculating for this value
            byte_count--;
        }

        if (endian == "little")
            tmp_bytes.reverse();
        array_bytes.push(...tmp_bytes);
    }

    return array_bytes;
}


export function MemoryCompare(arr1: number[], arr2: number[]): boolean
{
    if (arr1.length != arr2.length) return false;

    for (let i = 0; i < arr1.length; ++i)
    {
        if (arr1[i] != arr2[i])
            return false;
    }

    return true;
}