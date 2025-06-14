import time
import ctypes


# def trigger_breakpoint():
#     # Raise SIGTRAP, which GDB will catch
#     libc = ctypes.CDLL(None)
#     libc.raise_(5)  # 5 = SIGTRAP

def do_math(n):
    # trigger_breakpoint()
    total = 0
    for i in range(n):
        total += i * i
    return total

def main():
    result = 0
    while True:
        result = result + 1
        print(f"Computed result: {result}")
        print(f"Result : {id(result)}")
        time.sleep(1)

if __name__ == "__main__":
    main()