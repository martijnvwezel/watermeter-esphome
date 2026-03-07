"""Convert ESPHome log lines tagged with "log2csv" into CSV.

The old fixed-column format has been retired.  The new formatter simply scans
each input line that contains the literal `log2csv` tag, extracts all
`key:value` pairs and emits a row with one column per distinct key seen in
any line.  Columns are ordered by first appearance in the log stream, and the
`time` field (derived from the standard ESPHome timestamp) is always placed
first.

Usage:
    python tools/log_to_csv.py path/to/log.txt > out.csv
    cat path/to/log.txt | python tools/log_to_csv.py
    python tools/log_to_csv.py --fill-empty-with-previous path/to/log.txt > out.csv
"""

import argparse
import csv
import re
import sys

LINE_RE = re.compile(
    r'^\[(?P<time>\d{2}:\d{2}:\d{2}\.\d{3})\].*\[(?P<type>\w+):\s*\d+\]:\s*(?P<msg>.*)$'
)
KV_RE = re.compile(r'([a-z0-9_]+):\s*([\-\d\.]+)', re.IGNORECASE)


def parse_kv(msg):
    """Return ordered list of (key,value) tuples from the message."""
    return KV_RE.findall(msg)


def process_lines(lines, use_previous=False):
    """Scan lines and collect rows + dynamic header order.

    Unlike the previous version, multiple ``log2csv`` entries may describe a
    single sample.  Fields are merged until a line provides an ``al`` value,
    which triggers emission of the accumulated record.  This matches the log
    format where an ``al`` measurement finalises the group.

    Returns a tuple ``(headers, rows)`` where ``headers`` is a list of column
    names and ``rows`` is a list of dictionaries mapping column names to
    values.  Conversion to CSV is handled in ``main``.
    """

    headers = ['time']
    rows = []

    current = {}
    current_time = None

    for ln in lines:
        if 'log2csv' not in ln:
            continue

        m = LINE_RE.match(ln)
        if m:
            time = m.group('time')
            msg = m.group('msg')
        else:
            time = ''
            msg = ln

        kv_pairs = parse_kv(msg)
        if not kv_pairs:
            continue

        # flush when the record is complete (indicated by 'al')
        if ' al:' in ln and current:
            row = {'time': current_time}
            row.update(current)
            rows.append(row)
            if not use_previous:
                current = {}
                current_time = None

        # always update time to the latest seen; the flushed row uses this
        current_time = time

        for k, v in kv_pairs:
            if k not in headers:
                headers.append(k)
            try:
                val = float(v)
            except ValueError:
                val = v
            current[k] = val

    return headers, rows


def main():
    p = argparse.ArgumentParser(description="Convert ESPHome log to CSV using log2csv tags.")
    p.add_argument('infile', nargs='?', help='log file (defaults to stdin)')
    p.add_argument(
        '--use-previous',
        '-u',
        action='store_true',
        help='forward-fill empty values in a row with the previous row value for that column',
    )
    args = p.parse_args()

    fh = open(args.infile, 'r', encoding='utf-8') if args.infile else sys.stdin
    headers, rows = process_lines(fh, args.use_previous)

    writer = csv.writer(sys.stdout, lineterminator='\n')
    writer.writerow(headers)
    for r in rows:
        writer.writerow([r.get(h, '') for h in headers])

    if args.infile:
        fh.close()


if __name__ == '__main__':
    main()
