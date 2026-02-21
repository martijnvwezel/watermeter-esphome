"""Parse ESPHome log -> CSV with columns for raw, median, scaled (a/b/c) and min/max (a/b/c).

Usage:
  python algorithm/log_to_csv.py path/to/log.txt > out.csv
  cat path/to/log.txt | python algorithm/log_to_csv.py

Groups adjacent relevant log lines (org, sensor, raw/scaled/calibration/phase/results) into a single CSV row.
"""
import argparse
import csv
import re
import sys

LINE_RE = re.compile(r'^\[(?P<time>\d{2}:\d{2}:\d{2}\.\d{3})\].*\[(?P<type>\w+):\s*\d+\]:\s*(?P<msg>.*)$')
KV_RE = re.compile(r'([a-z0-9_]+):\s*([-\d.]+)', re.IGNORECASE)

HEADERS = [
    'time',
    'raw_a', 'raw_b', 'raw_c',
    'scaled_a', 'scaled_b', 'scaled_c',
    'org_al', 'org_bl', 'org_cl', 'org_ad', 'org_bd', 'org_cd',
    'min_a', 'min_b', 'min_c',
    'max_a', 'max_b', 'max_c',
    'prev_phase', 'cand_phase', 'est_phase', 'alt_phase', 'pn0', 'pn1', 'pn2',
    # results from computation
    'consumption_lifetime', 'consumption_since_restart', 'consumption_current', 'consumption_previous',
    'flow_rate_current', 'flow_rate_previous',
]

def parse_kv(msg):
    return {k: float(v) for k, v in KV_RE.findall(msg)}

def trip(kv, *keys):
    return tuple(kv[k] for k in keys) if all(k in kv for k in keys) else None

def process_lines(lines):
    DEFAULT_TRIPLE = (None, None, None)

    def make_current():
        return {
            'raw': DEFAULT_TRIPLE, 'scaled': DEFAULT_TRIPLE,
            'org_l': DEFAULT_TRIPLE, 'org_d': DEFAULT_TRIPLE,
            'min': DEFAULT_TRIPLE, 'max': DEFAULT_TRIPLE,
            'prev_phase': None, 'cand_phase': None, 'est_phase': None, 'alt_phase': None, 'pn': DEFAULT_TRIPLE,
            # results fields
            'consumption_lifetime': None, 'consumption_since_restart': None,
            'consumption_current': None, 'consumption_previous': None,
            'flow_rate_current': None, 'flow_rate_previous': None,
        }

    current = make_current()
    current_time = None

    def flush():
        nonlocal current, current_time
        if current['raw'][0] is None:
            current = make_current()
            current_time = None
            return None
        row = [current_time or '',
               *current['raw'], *current['scaled'],
               *current['org_l'], *current['org_d'],
               *current['min'], *current['max'],
               current['prev_phase'], current['cand_phase'], current['est_phase'], current['alt_phase'], *current['pn'],
               current['consumption_lifetime'], current['consumption_since_restart'],
               current['consumption_current'], current['consumption_previous'],
               current['flow_rate_current'], current['flow_rate_previous']]

        current = make_current()
        current_time = None
        return row

    for ln in lines:
        m = LINE_RE.match(ln)
        if not m:
            continue

        typ = m.group('type')
        # accept 'phase', 'org', 'sensor' and 'results' as relevant now — do NOT flush on unrelated lines
        if typ not in ('raw', 'calibration', 'scaled', 'phase', 'org', 'sensor', 'results'):
            continue

        kv = parse_kv(m.group('msg'))

        if typ == 'org':
            if current['org_l'][0] is not None:
                r = flush()
                if r:
                    yield r

            current_time = m.group('time')

            t_l = trip(kv, 'al', 'bl', 'cl')
            if t_l:
                current['org_l'] = t_l
            t_d = trip(kv, 'ad', 'bd', 'cd')
            if t_d:
                current['org_d'] = t_d
            continue



        # 'sensor' lines in the new log include both a/b/c (raw) and al/ad (org) on one line
        if typ == 'sensor':
            # parse org values if present
            t_l = trip(kv, 'al', 'bl', 'cl')
            if t_l:
                current['org_l'] = t_l
            t_d = trip(kv, 'ad', 'bd', 'cd')
            if t_d:
                current['org_d'] = t_d

            # parse raw a/b/c if present (acts like 'raw')
            t = trip(kv, 'a', 'b', 'c')
            if t:
                # new raw -> flush previous record
                if current['raw'][0] is not None:
                    r = flush()
                    if r:
                        yield r
                current_time = m.group('time')
                current['raw'] = t
            continue


        # unified handling for raw / median /scaled
        if typ in ('raw', 'scaled'):
            t = trip(kv, 'a', 'b', 'c')
            if not t:
                continue

            # new raw -> flush previous record (raw triggers record start)
            if typ == 'raw' and current['raw'][0] is not None:
                r = flush()
                if r:
                    yield r
            # set time when raw starts a record
            if typ == 'raw' and current_time is None:
                current_time = m.group('time')

            current[typ] = t
            continue

        # handle phase: attach only if a raw record has started;
        # do not flush here — the record will be flushed on next raw or at EOF
        if typ == 'phase':
            # only attach phase info to an active record started by `raw`
            if current['raw'][0] is None:
                continue

            msg = m.group('msg')

            # estimated phase: accept old `phase`, new `cand_phase`, `est_phase`, `alt_phase`
            for phase_key in ('phase', 'prev_phase', 'cand_phase', 'est_phase', 'alt_phase'):
                if phase_key in kv:
                    current[phase_key] = int(kv[phase_key]) if float(kv[phase_key]).is_integer() else kv[phase_key]

            # pn triple: prefer pn0/pn1/pn2 keys, otherwise parse `pn:` followed by three numbers
            tpn = trip(kv, 'pn0', 'pn1', 'pn2')
            if tpn:
                current['pn'] = tpn
            else:
                m_pn = re.search(r'\bpn:\s*([\-\d\.]+)\s+([\-\d\.]+)\s+([\-\d\.]+)', msg)
                if m_pn:
                    current['pn'] = (float(m_pn.group(1)), float(m_pn.group(2)), float(m_pn.group(3)))
                elif 'pn' in kv:
                    # fallback: single pn value captured by KV_RE
                    current['pn'] = (kv['pn'], None, None)
            continue

        if typ == 'results':
            # only attach results to an active record started by `raw`
            if current['raw'][0] is None:
                continue
            for rk in ('consumption_lifetime', 'consumption_since_restart', 'consumption_current', 'consumption_previous', 'flow_rate_current', 'flow_rate_previous'):
                if rk in kv:
                    current[rk] = kv[rk]
            continue

        if typ == 'calibration':
            tmin = trip(kv, 'a_min', 'b_min', 'c_min')
            if tmin:
                current['min'] = tmin
            tmax = trip(kv, 'a_max', 'b_max', 'c_max')
            if tmax:
                current['max'] = tmax
            continue

    r = flush()
    if r:
        yield r

def main():
    p = argparse.ArgumentParser(description="Convert ESPHome sensor log to CSV (raw/median/scaled a/b/c + min/max).")
    p.add_argument('infile', nargs='?', help='log file (defaults to stdin)')
    args = p.parse_args()

    fh = open(args.infile, 'r', encoding='utf-8') if args.infile else sys.stdin
    writer = csv.writer(sys.stdout, lineterminator='\n')
    writer.writerow(HEADERS)
    for row in process_lines(fh):
        fields = ["" if v is None else v for v in row]
        if not any(fields):
            continue
        writer.writerow(fields)
    if args.infile:
        fh.close()

if __name__ == '__main__':
    main()
