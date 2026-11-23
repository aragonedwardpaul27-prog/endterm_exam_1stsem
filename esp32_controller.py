import tkinter as tk
from tkinter import ttk
import serial
import serial.tools.list_ports

class ESP32App:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32")
        self.root.minsize(220, 240)
        self.root.resizable(True, True)

        # Configure grid layout so widgets scale cleanly with window size
        self.root.columnconfigure(0, weight=1)
        for row in range(6):
            self.root.rowconfigure(row, weight=1)

        self.ser = None
        self.poll_job = None

        # Port Selection & Connect
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(root, textvariable=self.port_var, state="readonly")
        self.port_combo.grid(row=0, column=0, padx=16, pady=(12, 6), sticky="ew")
        self.refresh_ports()

        self.conn_btn = ttk.Button(root, text="Connect", command=self.toggle_conn)
        self.conn_btn.grid(row=1, column=0, padx=16, pady=6, sticky="ew")

        # Current State Display
        self.state_var = tk.StringVar(value="State: DISCONNECTED")
        self.state_label = ttk.Label(root, textvariable=self.state_var, anchor="center", font=("TkDefaultFont", 10, "bold"))
        self.state_label.grid(row=2, column=0, padx=16, pady=6, sticky="ew")

        # Mode Buttons
        ttk.Button(root, text="Blink (1)", command=lambda: self.send("1")).grid(row=3, column=0, padx=16, pady=5, sticky="nsew")
        ttk.Button(root, text="Alternate (2)", command=lambda: self.send("2")).grid(row=4, column=0, padx=16, pady=5, sticky="nsew")
        ttk.Button(root, text="Running (3)", command=lambda: self.send("3")).grid(row=5, column=0, padx=16, pady=(5, 14), sticky="nsew")

    def refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_combo['values'] = ports
        if ports:
            self.port_combo.current(0)

    def toggle_conn(self):
        if not self.ser:
            try:
                self.ser = serial.Serial(self.port_var.get(), 115200, timeout=0.05)
                self.conn_btn.config(text="Disconnect")
                self.port_combo.config(state="disabled")
                self.state_var.set("State: CONNECTED")
                self.send("?")
                self.poll_serial()
            except Exception:
                self.ser = None
                self.state_var.set("State: ERROR")
        else:
            if self.poll_job:
                self.root.after_cancel(self.poll_job)
                self.poll_job = None
            self.ser.close()
            self.ser = None
            self.conn_btn.config(text="Connect")
            self.port_combo.config(state="readonly")
            self.state_var.set("State: DISCONNECTED")

    def send(self, val):
        if self.ser and self.ser.is_open:
            self.ser.write(f"{val}\n".encode())

    def poll_serial(self):
        if self.ser and self.ser.is_open:
            try:
                while self.ser.in_waiting:
                    line = self.ser.readline().decode("utf-8", errors="ignore").strip()
                    if line.startswith("STATE:"):
                        state_name = line.split(":", 1)[1].strip()
                        self.state_var.set(f"State: {state_name}")
            except Exception:
                pass
            self.poll_job = self.root.after(100, self.poll_serial)

if __name__ == "__main__":
    root = tk.Tk()
    ESP32App(root)
    root.mainloop()