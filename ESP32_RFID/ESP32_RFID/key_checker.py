# chat gpt generate this
# just to check the key in my flash is correct or not
# output: Private key content matches the binary data.

def hex_str_to_bytes(hex_str):
    return bytes.fromhex(hex_str)

def write_bytes_to_file(bytes_data, filename):
    with open(filename, 'wb') as f:
        f.write(bytes_data)

hex_str = "ESP's hex output here (my RSA private key)"
binary_data = hex_str_to_bytes(hex_str)

write_bytes_to_file(binary_data, "output.bin")

with open("private_key.der", "rb") as f:
    private_key_content = f.read()

with open("output.bin", "rb") as f:
    binary_data = f.read()

if private_key_content == binary_data:
    print("Private key content matches the binary data.")
else:
    print("Private key content does not match the binary data.")
