async function Sleep(delay: number)
{
    await new Promise(r => setTimeout(r, delay));
}

export default Sleep;