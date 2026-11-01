"""Abre os dois monitores em uma unica janela externa do Terminal no macOS."""

import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile


PROJECT = Path(__file__).resolve().parent.parent
PIO = Path.home() / ".platformio/penv/bin/pio"
TITLE = "Monitores RF24 - " + hashlib.sha256(str(PROJECT).encode()).hexdigest()[:8]


def screen_config():
    # O screen incluido no macOS divide a janela em cima/baixo.
    # Cada area executa um monitor independente; fechar a janela encerra ambos.
    lines = ["startup_message off", "autodetach off", "caption always '%t'", "defscrollback 5000"]
    for index, port in enumerate(("120", "130")):
        if index:
            lines.extend(["split", "focus down"])
        args = [str(PIO), "device", "monitor", "--port", f"/dev/cu.usbserial-{port}",
                "--baud", "115200", "--dtr", "0", "--rts", "0"]
        # Mantem a area aberta para que erros de conexao possam ser lidos.
        command = ("cd " + shlex.quote(str(PROJECT)) + " && " + shlex.join(args)
                   + "; printf '\\nMonitor encerrado. Pressione Enter para fechar esta area.'; read -r resposta")
        lines.append("screen -t USB" + port + " /bin/zsh -c " + json.dumps(command, ensure_ascii=False))
    lines.extend(["focus up", "select 0"])
    return "\n".join(lines) + "\n"


APPLESCRIPT = '''
on run argv
    set windowTitle to item 1 of argv
    set monitorCommand to item 2 of argv
    tell application "/System/Applications/Utilities/Terminal.app"
        repeat with w in windows
            repeat with t in tabs of w
                if custom title of t is windowTitle then
                    set index of w to 1
                    activate
                    return
                end if
            end repeat
        end repeat
        set monitorTab to do script monitorCommand
        set custom title of monitorTab to windowTitle
        set number of columns of monitorTab to 140
        set number of rows of monitorTab to 44
        activate
    end tell
end run
'''


def main():
    config = screen_config()
    if "--check" in sys.argv:
        print(config)
        return
    if not PIO.is_file():
        raise SystemExit(f"PlatformIO nao encontrado: {PIO}")
    # O arquivo permanece disponivel ate o Terminal iniciar o screen.
    config_path = Path(tempfile.gettempdir()) / (TITLE.replace(" ", "_") + ".screenrc")
    config_path.write_text(config)
    command = shlex.join(["/usr/bin/screen", "-c", str(config_path), "-S", TITLE.replace(" ", "_")])
    subprocess.run(["/usr/bin/osascript", "-e", APPLESCRIPT, TITLE, command], check=True)


if __name__ == "__main__":
    main()
