# {{COMPONENT_TITLE}} Basic Example

Minimal example showing how to use the **{{COMPONENT_NAME}}** component.

## How to Use

### Build and Flash

```bash
idf.py -p PORT flash monitor
```

(Replace `PORT` with the serial port name, e.g., `/dev/ttyUSB0` or `COM3`.
To exit the serial monitor, type `Ctrl-]`.)

## Project Structure

```text
examples/basic/
├── CMakeLists.txt
├── sdkconfig.defaults
├── README.md
└── main/
    ├── CMakeLists.txt
    ├── idf_component.yml
    └── main.c
```
