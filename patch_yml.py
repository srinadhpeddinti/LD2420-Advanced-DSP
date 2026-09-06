with open(".github/workflows/build.yml", 'r') as f:
    content = f.read()

content = content.replace("        sketch-paths: |\n          - firmware/LD2420_Pico_Ultimate\n", "        sketch-paths: |\n          - firmware/LD2420_Pico_Ultimate/\n")
with open(".github/workflows/build.yml", 'w') as f:
    f.write(content)
