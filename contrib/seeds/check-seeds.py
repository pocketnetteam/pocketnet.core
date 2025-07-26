#!/usr/bin/env python3
"""
Script to check node availability from nodes_main.txt
Проверяет доступность узлов из списка с помощью TCP-соединения
"""

import socket
import threading
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from typing import List, Tuple, Dict
import argparse
import sys

def check_node(host: str, port: int, timeout: int = 5) -> Tuple[str, int, bool, str]:
    """
    Check if a node is reachable via TCP connection
    
    Args:
        host: IP address or hostname
        port: Port number
        timeout: Connection timeout in seconds
        
    Returns:
        Tuple of (host, port, is_available, error_message)
    """
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.settimeout(timeout)
            result = sock.connect_ex((host, port))
            if result == 0:
                return (host, port, True, "OK")
            else:
                return (host, port, False, f"Connection failed (error code: {result})")
    except socket.timeout:
        return (host, port, False, "Timeout")
    except socket.gaierror as e:
        return (host, port, False, f"DNS resolution failed: {e}")
    except Exception as e:
        return (host, port, False, f"Error: {e}")

def parse_nodes_file(filename: str) -> List[Tuple[str, int]]:
    """
    Parse nodes file and extract host:port pairs
    
    Args:
        filename: Path to nodes file
        
    Returns:
        List of (host, port) tuples
    """
    nodes = []
    try:
        with open(filename, 'r') as f:
            for line_num, line in enumerate(f, 1):
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                
                # Handle spaces before port (like "213.109.13.125 :37070")
                line = line.replace(' :', ':')
                
                if ':' in line:
                    try:
                        host, port_str = line.rsplit(':', 1)
                        port = int(port_str.strip())
                        host = host.strip()
                        nodes.append((host, port))
                    except ValueError:
                        print(f"Warning: Invalid format on line {line_num}: {line}", file=sys.stderr)
                else:
                    print(f"Warning: No port found on line {line_num}: {line}", file=sys.stderr)
    except FileNotFoundError:
        print(f"Error: File {filename} not found", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error reading file {filename}: {e}", file=sys.stderr)
        sys.exit(1)
    
    return nodes

def check_nodes_parallel(nodes: List[Tuple[str, int]], timeout: int = 5, max_workers: int = 50) -> Dict:
    """
    Check multiple nodes in parallel
    
    Args:
        nodes: List of (host, port) tuples
        timeout: Connection timeout per node
        max_workers: Maximum number of concurrent connections
        
    Returns:
        Dictionary with results
    """
    results = {
        'available': [],
        'unavailable': [],
        'total': len(nodes),
        'available_count': 0,
        'unavailable_count': 0
    }
    
    print(f"Checking {len(nodes)} nodes with timeout {timeout}s...")
    print("=" * 60)
    
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        # Submit all tasks
        future_to_node = {
            executor.submit(check_node, host, port, timeout): (host, port)
            for host, port in nodes
        }
        
        # Process completed tasks
        for future in as_completed(future_to_node):
            host, port, is_available, message = future.result()
            
            if is_available:
                results['available'].append((host, port, message))
                results['available_count'] += 1
                status = "[V] AVAILABLE"
            else:
                results['unavailable'].append((host, port, message))
                results['unavailable_count'] += 1
                status = "[X] UNAVAILABLE"
            
            print(f"{status:12} {host:15}:{port:5} - {message}")
    
    return results

def print_summary(results: Dict):
    """Print summary of results"""
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)
    print(f"Total nodes checked: {results['total']}")
    print(f"Available nodes:     {results['available_count']} ({results['available_count']/results['total']*100:.1f}%)")
    print(f"Unavailable nodes:   {results['unavailable_count']} ({results['unavailable_count']/results['total']*100:.1f}%)")
    
    if results['available']:
        print(f"\nAvailable nodes ({len(results['available'])}):")
        for host, port, _ in sorted(results['available']):
            print(f"  {host}:{port}")
    
    if results['unavailable']:
        print(f"\nUnavailable nodes ({len(results['unavailable'])}):")
        for host, port, message in sorted(results['unavailable']):
            print(f"  {host}:{port} - {message}")

def main():
    parser = argparse.ArgumentParser(description='Check node availability from nodes list')
    parser.add_argument('filename', nargs='?', default='nodes_main.txt',
                       help='Path to nodes file (default: nodes_main.txt)')
    parser.add_argument('-t', '--timeout', type=int, default=15,
                       help='Connection timeout in seconds (default: 15)')
    parser.add_argument('-w', '--workers', type=int, default=50,
                       help='Maximum concurrent connections (default: 50)')
    parser.add_argument('--available-only', action='store_true',
                       help='Show only available nodes')
    parser.add_argument('--unavailable-only', action='store_true',
                       help='Show only unavailable nodes')
    
    args = parser.parse_args()
    
    # Parse nodes file
    nodes = parse_nodes_file(args.filename)
    if not nodes:
        print("No valid nodes found in file", file=sys.stderr)
        sys.exit(1)
    
    # Check nodes
    start_time = time.time()
    results = check_nodes_parallel(nodes, args.timeout, args.workers)
    end_time = time.time()
    
    print(f"\nCompleted in {end_time - start_time:.2f} seconds")
    
    # Print summary
    if not args.available_only and not args.unavailable_only:
        print_summary(results)
    elif args.available_only:
        print(f"\nAvailable nodes ({results['available_count']}):")
        for host, port, _ in sorted(results['available']):
            print(f"{host}:{port}")
    elif args.unavailable_only:
        print(f"\nUnavailable nodes ({results['unavailable_count']}):")
        for host, port, message in sorted(results['unavailable']):
            print(f"{host}:{port} - {message}")

if __name__ == "__main__":
    main()