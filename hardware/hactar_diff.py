import os

path = "EV13"

input("Oldify the pdf?")
os.system(f"rm ./{path}/old.pdf")
os.system(f"cp ./{path}/'EV13 Board Design.pdf' ./{path}/old.pdf")

input("Run the image diff?")
os.system(f"magick convert -density 300 ./{path}/old.pdf -quality 50 -depth 16 ./{path}/new.png")
os.system(f"magick convert -density 300 ./{path}/'EV13 Board Design.pdf' -quality 50 -depth 16 ./{path}/new.png")
os.system(f"magick compare ./{path}/old.png ./{path}/new.png ./{path}/diff.png")
