# Get pyserial from our local copy
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "vendor"))
import serial
import serial.tools.list_ports
