async function Sleep(delay)
{
    await new Promise(r => setTimeout(r, delay));
}

export default Sleep;