import argparse
import http.client

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("payload", help="Path to riscvm payload.bin")
    parser.add_argument("--trace", help="Enable tracing")
    args = parser.parse_args()

    print(f"payload: {args.payload}")

    with open(args.payload, "rb") as f:
        data = f.read()

    # TODO: validate this is a valid rv64 payload (check relocations)
    data = b"RV64" + data

    conn = http.client.HTTPConnection("127.0.0.1", 13337)

    headers = {
        "Content-Type": "application/octet-stream",
        "Content-Length": str(len(data))
    }

    conn.request("POST", "/riscvm", body=data, headers=headers)
    response = conn.getresponse()

    print(response.status, response.reason)
    print(response.read().decode())

    conn.close()

if __name__ == "__main__":
    main()
