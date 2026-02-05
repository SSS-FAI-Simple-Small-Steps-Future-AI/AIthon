async def pinger(count):
    i = 0
    while i < count:
        print(i)
        i = i + 1
    return count

async def main():
    result = await pinger(5)
    print(result)

main()