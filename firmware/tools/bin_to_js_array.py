

firmware = open("../ui/build/ui.bin", "rb").read()

js_array = open("../web/ui_bin.js", "w")

js_array.write("const bin = [")

js_array.write(str(firmware[0]))
for i in range(1, len(firmware)):
    js_array.write(f", {str(firmware[i])}")

js_array.write("];")

