"""
F1 start graphic for the STM32 reaction speed tester.

Run:
    python f1_go_graphic.py COM3

Change COM3 to the ST-LINK Virtual COM Port shown in Device Manager.
If pyserial is not installed, the window still works: press G to launch the car.
"""

import queue
import sys
import threading
import time
import tkinter as tk

BAUD_RATE = 9600
WINDOW_WIDTH = 1000
WINDOW_HEIGHT = 520


class SerialReader(threading.Thread):
    def __init__(self, port, outbox):
        super().__init__(daemon=True)
        self.port = port
        self.outbox = outbox
        self.running = True

    def run(self):
        try:
            import serial
        except ImportError:
            self.outbox.put(("status", "pyserial not installed. Press G to test animation."))
            return

        try:
            with serial.Serial(self.port, BAUD_RATE, timeout=0.1) as ser:
                self.outbox.put(("status", f"Connected: {self.port} @ {BAUD_RATE}"))
                while self.running:
                    line = ser.readline().decode(errors="ignore").strip()
                    if line:
                        self.outbox.put(("serial", line))
        except Exception as exc:
            self.outbox.put(("status", f"Serial error: {exc}. Press G to test animation."))


class F1App:
    def __init__(self, root, port):
        self.root = root
        self.root.title("STM32 F1 Reaction Start")
        self.root.resizable(False, False)

        self.canvas = tk.Canvas(root, width=WINDOW_WIDTH, height=WINDOW_HEIGHT, bg="#17202a")
        self.canvas.pack()

        self.messages = queue.Queue()
        self.serial_reader = SerialReader(port, self.messages) if port else None

        self.car_x = 90
        self.car_y = 340
        self.car_speed = 0
        self.launching = False
        self.status_text = "Waiting for STM32... Press G to test."
        self.last_serial_text = ""
        self.flash_until = 0
        self.result_text = ""
        self.active_light = None
        self.reaction_time_text = "--"
        self.rankings = []

        self.root.bind("<g>", lambda event: self.launch_car())
        self.root.bind("<G>", lambda event: self.launch_car())
        self.root.bind("<r>", lambda event: self.reset_car())
        self.root.bind("<R>", lambda event: self.reset_car())
        self.root.protocol("WM_DELETE_WINDOW", self.close)

        if self.serial_reader:
            self.serial_reader.start()
        else:
            self.status_text = "No COM port given. Press G to test animation."

        self.tick()

    def close(self):
        if self.serial_reader:
            self.serial_reader.running = False
        self.root.destroy()

    def launch_car(self):
        self.car_x = 90
        self.car_speed = 7
        self.launching = True
        self.flash_until = time.time() + 0.7
        self.result_text = "GO!"
        self.active_light = "GO"

    def reset_car(self):
        self.car_x = 90
        self.car_speed = 0
        self.launching = False
        self.result_text = ""
        self.active_light = None
        self.reaction_time_text = "--"

    def handle_serial_line(self, line):
        self.last_serial_text = line
        if line == "Ready":
            self.reset_car()
        elif line in ("3", "2", "1"):
            self.active_light = line
            self.result_text = line
        elif line == "GO!":
            self.launch_car()
        elif "Reaction Time:" in line:
            self.result_text = line
            self.reaction_time_text = line.replace("Reaction Time:", "").replace("sec", "").strip() + " sec"
            self.add_ranking_time(self.reaction_time_text)
            self.active_light = None
        elif line in ("Success!", "Fail!", "Too early!"):
            self.result_text = line
            self.active_light = None
            if line != "Success!":
                self.car_speed = 0

    def process_messages(self):
        while True:
            try:
                kind, value = self.messages.get_nowait()
            except queue.Empty:
                break

            if kind == "serial":
                self.handle_serial_line(value)
            elif kind == "status":
                self.status_text = value

    def tick(self):
        self.process_messages()

        if self.launching:
            self.car_speed = min(self.car_speed + 0.22, 24)
            self.car_x += self.car_speed
            if self.car_x > WINDOW_WIDTH + 180:
                self.launching = False
                self.car_speed = 0

        self.draw()
        self.root.after(16, self.tick)

    def draw(self):
        self.canvas.delete("all")
        self.draw_background()
        self.draw_start_lights()
        self.draw_reaction_panel()
        self.draw_ranking_panel()
        self.draw_track()
        self.draw_car(self.car_x, self.car_y)
        self.draw_overlay()

    def draw_background(self):
        self.canvas.create_rectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, fill="#17202a", outline="")
        self.canvas.create_rectangle(0, 0, WINDOW_WIDTH, 210, fill="#22313f", outline="")
        self.canvas.create_text(
            40, 34, anchor="w", text="STM32 F1 Reaction Start",
            fill="white", font=("Segoe UI", 24, "bold")
        )
        self.canvas.create_text(
            40, 70, anchor="w", text=self.status_text,
            fill="#cfd8dc", font=("Segoe UI", 12)
        )
        self.canvas.create_text(
            40, 96, anchor="w", text=f"Last Serial: {self.last_serial_text}",
            fill="#9fb3c8", font=("Consolas", 12)
        )

    def draw_start_lights(self):
        lights = [
            ("3", "PC8", "#ff2e2e"),
            ("2", "PC6", "#ffb020"),
            ("1", "PC5", "#ffe066"),
            ("GO", "PB14", "#2ce66b"),
        ]
        x0 = 650
        y0 = 36

        self.canvas.create_text(
            x0 - 12, y0 + 18, anchor="e", text="Board LEDs",
            fill="#cfd8dc", font=("Segoe UI", 11, "bold")
        )

        for i, (key, pin_label, on_color) in enumerate(lights):
            x = x0 + i * 76
            is_on = self.active_light == key
            fill = on_color if is_on else "#3f474f"
            glow = on_color if is_on else "#26313a"

            if is_on:
                self.canvas.create_oval(x - 5, y0 - 5, x + 45, y0 + 45, fill=glow, outline="")

            self.canvas.create_oval(x, y0, x + 40, y0 + 40, fill=fill, outline="#111820", width=3)
            self.canvas.create_text(
                x + 20, y0 + 55, text=key,
                fill="#ffffff" if is_on else "#b0bec5", font=("Segoe UI", 11, "bold")
            )
            self.canvas.create_text(
                x + 20, y0 + 72, text=pin_label,
                fill="#8fa2b4", font=("Segoe UI", 9)
            )

    def draw_reaction_panel(self):
        x0 = 650
        y0 = 122
        width = 310
        height = 72

        self.canvas.create_rectangle(x0, y0, x0 + width, y0 + height, fill="#151d26", outline="#36495c", width=2)
        self.canvas.create_text(
            x0 + 18, y0 + 18, anchor="w", text="Reaction Time",
            fill="#9fb3c8", font=("Segoe UI", 11, "bold")
        )
        self.canvas.create_text(
            x0 + width - 18, y0 + 45, anchor="e", text=self.reaction_time_text,
            fill="#ffd166", font=("Consolas", 24, "bold")
        )

    def add_ranking_time(self, time_text):
        try:
            seconds = float(time_text.replace("sec", "").strip())
        except ValueError:
            return

        self.rankings.append(seconds)
        self.rankings.sort()
        self.rankings = self.rankings[:5]

    def draw_ranking_panel(self):
        x0 = 40
        y0 = 128
        width = 360
        height = 150
        medals = [("Gold", "#ffd700"), ("Silver", "#c0c0c0"), ("Bronze", "#cd7f32")]

        self.canvas.create_rectangle(x0, y0, x0 + width, y0 + height, fill="#151d26", outline="#36495c", width=2)
        self.canvas.create_text(
            x0 + 18, y0 + 20, anchor="w", text="Reaction Ranking",
            fill="#ffffff", font=("Segoe UI", 13, "bold")
        )

        for index in range(3):
            row_y = y0 + 52 + index * 30
            label, color = medals[index]

            self.canvas.create_oval(x0 + 20, row_y - 11, x0 + 42, row_y + 11, fill=color, outline="#111820")
            self.canvas.create_text(
                x0 + 31, row_y, text=str(index + 1),
                fill="#111820", font=("Segoe UI", 9, "bold")
            )

            if index < len(self.rankings):
                time_text = f"{self.rankings[index]:.3f} sec"
            else:
                time_text = "--"

            self.canvas.create_text(
                x0 + 60, row_y, anchor="w", text=f"{label}",
                fill="#cfd8dc", font=("Segoe UI", 11, "bold")
            )
            self.canvas.create_text(
                x0 + width - 20, row_y, anchor="e", text=time_text,
                fill="#ffd166" if index == 0 and index < len(self.rankings) else "#d5dde5",
                font=("Consolas", 15, "bold")
            )

    def draw_track(self):
        self.canvas.create_rectangle(0, 300, WINDOW_WIDTH, WINDOW_HEIGHT, fill="#3b3f45", outline="")
        self.canvas.create_rectangle(0, 292, WINDOW_WIDTH, 305, fill="#1f2328", outline="")
        self.canvas.create_line(0, 405, WINDOW_WIDTH, 405, fill="#f4f4f4", width=4, dash=(34, 26))
        self.canvas.create_rectangle(210, 260, 222, WINDOW_HEIGHT, fill="white", outline="")

        for y in range(260, WINDOW_HEIGHT, 26):
            fill = "#111" if (y // 26) % 2 == 0 else "#fff"
            self.canvas.create_rectangle(222, y, 246, y + 26, fill=fill, outline="")
            self.canvas.create_rectangle(186, y, 210, y + 26, fill="#fff" if fill == "#111" else "#111", outline="")

    def draw_car(self, x, y):
        red = "#e10600"
        dark_red = "#8b0015"
        yellow = "#ffd166"
        carbon = "#111318"
        tire = "#090909"

        # ground shadow
        self.canvas.create_oval(x - 38, y + 63, x + 318, y + 104, fill="#111820", outline="")

        # speed lines
        if self.launching:
            for i in range(7):
                x1 = x - 90 - i * 42
                self.canvas.create_line(x1, y + 14 + i * 7, x1 + 75, y + 14 + i * 7, fill="#f7d794", width=3)

        # rear wing assembly
        self.canvas.create_rectangle(x - 50, y + 4, x + 18, y + 17, fill=carbon, outline="")
        self.canvas.create_rectangle(x - 56, y + 34, x + 22, y + 48, fill=carbon, outline="")
        self.canvas.create_rectangle(x - 38, y - 4, x - 26, y + 58, fill=dark_red, outline="")
        self.canvas.create_rectangle(x + 8, y + 0, x + 17, y + 54, fill=dark_red, outline="")

        # floor and sidepod
        self.canvas.create_polygon(
            x - 4, y + 34, x + 152, y + 18, x + 238, y + 29, x + 205, y + 57, x + 12, y + 60,
            fill="#20242b", outline=""
        )
        self.canvas.create_polygon(
            x + 16, y + 20, x + 116, y + 2, x + 185, y + 16, x + 154, y + 50, x + 25, y + 48,
            fill=red, outline=""
        )
        self.canvas.create_polygon(
            x + 42, y + 27, x + 118, y + 18, x + 147, y + 28, x + 123, y + 41, x + 52, y + 39,
            fill="#b5001b", outline=""
        )

        # cockpit, halo, and driver helmet
        self.canvas.create_oval(x + 99, y - 16, x + 134, y + 18, fill=carbon, outline="")
        self.canvas.create_oval(x + 108, y - 22, x + 128, y - 2, fill=yellow, outline="")
        self.canvas.create_arc(x + 83, y - 28, x + 151, y + 34, start=25, extent=130, outline=carbon, width=5)

        # engine cover and shark fin
        self.canvas.create_polygon(
            x + 86, y - 6, x + 154, y - 30, x + 195, y + 12, x + 133, y + 18,
            fill=red, outline=""
        )
        self.canvas.create_polygon(x + 136, y - 28, x + 160, y - 54, x + 174, y - 12, fill=dark_red, outline="")

        # long nose cone
        self.canvas.create_polygon(
            x + 158, y + 18, x + 248, y + 13, x + 305, y + 25, x + 244, y + 35, x + 157, y + 39,
            fill=red, outline=""
        )
        self.canvas.create_polygon(x + 181, y + 21, x + 268, y + 22, x + 247, y + 29, x + 177, y + 31, fill="#f8f8f8", outline="")

        # front wing planes and endplates
        self.canvas.create_rectangle(x + 274, y + 1, x + 355, y + 14, fill=carbon, outline="")
        self.canvas.create_rectangle(x + 276, y + 44, x + 358, y + 58, fill=carbon, outline="")
        self.canvas.create_rectangle(x + 346, y - 3, x + 362, y + 63, fill=dark_red, outline="")
        self.canvas.create_line(x + 282, y + 17, x + 350, y + 5, fill="#6f7782", width=3)
        self.canvas.create_line(x + 285, y + 41, x + 352, y + 54, fill="#6f7782", width=3)

        # suspension arms
        self.canvas.create_line(x + 184, y + 40, x + 230, y + 74, fill=carbon, width=4)
        self.canvas.create_line(x + 190, y + 21, x + 230, y + 61, fill=carbon, width=4)
        self.canvas.create_line(x + 28, y + 43, x + 62, y + 76, fill=carbon, width=4)
        self.canvas.create_line(x + 38, y + 21, x + 62, y + 63, fill=carbon, width=4)

        # wheels are drawn last so they sit in front of bodywork
        wheel_spin = self.launching
        self.draw_wheel(x + 64, y + 72, 32, wheel_spin)
        self.draw_wheel(x + 232, y + 72, 34, wheel_spin)

    def draw_wheel(self, x, y, radius=30, spinning=False):
        self.canvas.create_oval(x - radius, y - radius, x + radius, y + radius, fill="#050505", outline="")
        self.canvas.create_oval(x - radius + 5, y - radius + 5, x + radius - 5, y + radius - 5, fill="#171717", outline="#2d2d2d")
        self.canvas.create_oval(x - 12, y - 12, x + 12, y + 12, fill="#b0b6bd", outline="")
        if spinning:
            self.canvas.create_arc(x - radius + 8, y - radius + 8, x + radius - 8, y + radius - 8, start=20, extent=120, outline="#d7dde2", width=3)
            self.canvas.create_arc(x - radius + 8, y - radius + 8, x + radius - 8, y + radius - 8, start=200, extent=120, outline="#d7dde2", width=3)
        else:
            for angle in (0, 60, 120):
                if angle == 0:
                    self.canvas.create_line(x - 18, y, x + 18, y, fill="#d7dde2", width=2)
                elif angle == 60:
                    self.canvas.create_line(x - 10, y - 16, x + 10, y + 16, fill="#d7dde2", width=2)
                else:
                    self.canvas.create_line(x - 10, y + 16, x + 10, y - 16, fill="#d7dde2", width=2)

    def draw_overlay(self):
        if self.result_text:
            self.canvas.create_text(
                WINDOW_WIDTH // 2, 175, text=self.result_text,
                fill="#ffd166", font=("Segoe UI", 38, "bold")
            )
        self.canvas.create_text(
            WINDOW_WIDTH - 30, WINDOW_HEIGHT - 24, anchor="e",
            text="G: test launch   R: reset",
            fill="#b0bec5", font=("Segoe UI", 11)
        )


def main():
    port = sys.argv[1] if len(sys.argv) >= 2 else ""
    root = tk.Tk()
    F1App(root, port)
    root.mainloop()


if __name__ == "__main__":
    main()
