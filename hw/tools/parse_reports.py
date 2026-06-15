#!/usr/bin/env python3
import sys
import re
from pathlib import Path

def parse_resource_utilization(content):
    """Extract resource utilization from main report"""
    print("\n=== Resource Utilization ===")
    
    # Logic row
    logic_match = re.search(r'Logic\s*</td>\s*<td[^>]*>\s*(\d+)/(\d+)\s*</td>\s*<td[^>]*>\s*(\d+)%', content, re.DOTALL)
    if logic_match:
        used, total, percent = logic_match.groups()
        print(f"Logic: {used}/{total} ({percent}%)")
    
    # LUT,ALU,ROM16 breakdown
    lut_alu_match = re.search(r'--LUT,ALU,ROM16\s*</td>\s*<td[^>]*>\s*(\d+)\((\d+)\s*LUT,\s*(\d+)\s*ALU,\s*(\d+)\s*ROM16\)', content, re.DOTALL)
    if lut_alu_match:
        total_logic, luts, alus, rom16 = lut_alu_match.groups()
        print(f"  LUTs: {luts}")
        print(f"  ALUs: {alus}")
        print(f"  ROM16: {rom16}")
    
    # SSRAM(RAM16)
    ssram_match = re.search(r'--SSRAM\(RAM16\)\s*</td>\s*<td[^>]*>\s*(\d+)', content, re.DOTALL)
    if ssram_match:
        print(f"  SSRAM: {ssram_match.group(1)}")
    
    # BSRAM
    bsram_match = re.search(r'BSRAM\s*</td>\s*<td[^>]*>(.*?)</td>\s*<td[^>]*>\s*(\d+)%', content, re.DOTALL)
    if bsram_match:
        bsram_data = bsram_match.group(1)
        bsram_percent = bsram_match.group(2)
        bsram_clean = re.sub(r'<br\s*/?\s*>', ', ', bsram_data).strip()
        bsram_clean = re.sub(r',\s*$', '', bsram_clean)
        print(f"  BSRAM: {bsram_clean} ({bsram_percent}%)")

def parse_max_frequency_table(content):
    """Extract data from Max Frequency Summary table"""    
    table_match = re.search(r'<h2><a name="Max_Frequency_Report">.*?</h2>\s*<table>(.*?)</table>', content, re.DOTALL)
    
    if table_match:
        table_content = table_match.group(1)
        row_pattern = r'<tr>\s*<td>(\d+)</td>\s*<td>([^<]+)</td>\s*<td>([^<]+)</td>\s*<td>([^<]+)</td>\s*<td>(\d+)</td>\s*<td>([^<]+)</td>\s*</tr>'
        
        found_data = False
        for match in re.finditer(row_pattern, table_content):
            no, clock_name, constraint, actual_fmax, logic_level, entity = match.groups()
            print(f"Clock: {clock_name} | Constraint: {constraint} | Actual: {actual_fmax} | Logic Level: {logic_level}")
            found_data = True
        
        if not found_data:
            print("No frequency data found")
    else:
        print("Max Frequency table not found")

def parse_critical_paths(content, path_type="Setup"):
    """Extract detailed critical path information"""
    
    table_name = f"{path_type}_Slack_Table"
    table_match = re.search(
        rf'<h3><a name="{table_name}">{path_type} Paths Table</a></h3>.*?<table.*?>(.*?)</table>', 
        content, re.DOTALL
    )
    
    if not table_match:
        return None
    
    table_content = table_match.group(1)
    
    # Extract all path rows - pattern may vary by Gowin version
    # Typical columns: No, Slack, From, To, Delay, etc.
    row_pattern = r'<tr>\s*<td>(\d+)</td>\s*<td>([^<]+)</td>\s*<td>([^<]*)</td>\s*<td>([^<]*)</td>'
    
    paths = []
    for match in re.finditer(row_pattern, table_content):
        path_no = match.group(1)
        slack = match.group(2).strip()
        from_node = match.group(3).strip() if match.group(3) else "N/A"
        to_node = match.group(4).strip() if match.group(4) else "N/A"
        
        # Clean up HTML entities and extra whitespace
        from_node = re.sub(r'\s+', ' ', from_node)
        to_node = re.sub(r'\s+', ' ', to_node)
        
        paths.append({
            'no': path_no,
            'slack': slack,
            'from': from_node,
            'to': to_node
        })
    
    return paths

def parse_timing_slack(content):
    """Extract timing slack and critical paths"""
    
    # Setup paths
    setup_table_match = re.search(r'<h3><a name="Setup_Slack_Table">Setup Paths Table</a></h3>.*?<table.*?>(.*?)</table>', content, re.DOTALL)
    
    if setup_table_match:
        setup_table = setup_table_match.group(1)
        setup_row = re.search(r'<tr>\s*<td>\d+</td>\s*<td>([^<]+)</td>', setup_table)
        if setup_row:
            print(f"Worst Setup Slack: {setup_row.group(1)} ns")
    
    # Hold paths
    hold_table_match = re.search(r'<h3><a name="Hold_Slack_Table">Hold Paths Table</a></h3>.*?<table.*?>(.*?)</table>', content, re.DOTALL)
    
    if hold_table_match:
        hold_table = hold_table_match.group(1)
        hold_row = re.search(r'<tr>\s*<td>\d+</td>\s*<td>([^<]+)</td>', hold_table)
        if hold_row:
            print(f"Worst Hold Slack: {hold_row.group(1)} ns")
    
    # Extract critical path details
    print("\n=== Critical Paths ===")
    
    setup_paths = parse_critical_paths(content, "Setup")
    if setup_paths and len(setup_paths) > 0:
        print(f"\nWorst Setup Path:")
        p = setup_paths[0]
        print(f"  Slack: {p['slack']} ns")
        print(f"  From:  {p['from']}")
        print(f"  To:    {p['to']}")
        
        # Show top 3 critical paths if available
        if len(setup_paths) > 1:
            print(f"\nTop {min(3, len(setup_paths))} Setup Paths:")
            for i, p in enumerate(setup_paths[:3], 1):
                print(f"  {i}. Slack: {p['slack']:>8} ns  |  {p['from'][:40]:40s} -> {p['to'][:40]:40s}")
    
    hold_paths = parse_critical_paths(content, "Hold")
    if hold_paths and len(hold_paths) > 0:
        print(f"\nWorst Hold Path:")
        p = hold_paths[0]
        print(f"  Slack: {p['slack']} ns")
        print(f"  From:  {p['from']}")
        print(f"  To:    {p['to']}")

def parse_tns_table(content):
    """Extract TNS data"""
    table_match = re.search(r'<h2><a name="Total_Negative_Slack_Report">.*?</h2>.*?<table.*?>(.*?)</table>', content, re.DOTALL)
    
    if table_match:
        table_content = table_match.group(1)
        
        setup_match = re.search(r'<td>([^<]+)</td>\s*<td>Setup</td>\s*<td>([^<]+)</td>', table_content)
        hold_match = re.search(r'<td>([^<]+)</td>\s*<td>Hold</td>\s*<td>([^<]+)</td>', table_content)
        
        if setup_match:
            print(f"Setup TNS: {setup_match.group(2)}")
        if hold_match:
            print(f"Hold TNS: {hold_match.group(2)}")

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 parse_reports.py <main_report_file> <timing_file>")
        sys.exit(1)
    
    main_report_file = Path(sys.argv[1])
    timing_file = Path(sys.argv[2])
    
    # Parse resource utilization
    if main_report_file.exists():
        try:
            with open(main_report_file, 'r') as f:
                content = f.read()
            parse_resource_utilization(content)
        except Exception as e:
            print(f"Error parsing main report: {e}")
    else:
        print(f"Main report not found: {main_report_file}")
    
    # Parse timing report
    if timing_file.exists():
        try:
            with open(timing_file, 'r') as f:
                content = f.read()
            
            print("\n=== Timing Analysis ===")
            parse_max_frequency_table(content)
            parse_timing_slack(content)
            parse_tns_table(content)
            
        except Exception as e:
            print(f"Error parsing timing file: {e}")
    else:
        print(f"Timing file not found: {timing_file}")

if __name__ == "__main__":
    main()