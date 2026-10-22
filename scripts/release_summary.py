#!/usr/bin/env python3
import argparse
import os
import re
import subprocess
import sys

def run_cmd(cmd):
    return subprocess.check_output(cmd, shell=True).decode('utf-8').strip()

def format_special_thanks(thanks, reason=None):
    if not thanks:
        return None
    thanks = thanks.strip()
    if thanks.startswith("###"):
        return thanks
    if thanks.startswith("Huge thanks") or "\n" in thanks:
        return thanks
    name = thanks.strip("*")
    if reason:
        reason = reason.strip()
        if not reason.endswith("!"):
            reason += "!"
        return f"Huge thanks to **{name}** for {reason}"
    else:
        return f"Huge thanks to **{name}**!"

def main():
    parser = argparse.ArgumentParser(description="Generate release summary")
    parser.add_argument("range", nargs="?", default=None, help="Git commit or tag range (e.g. v0.1.0..v0.1.1)")
    parser.add_argument("--thanks", "--special-thanks", dest="thanks", default=None, help="Person to thank or full thanks message")
    parser.add_argument("--for", "--reason", dest="thanks_reason", default=None, help="Reason or contribution to thank the person for")
    parser.add_argument("--dry-run", action="store_true", help="Print summary without updating GitHub release")
    args = parser.parse_args()

    if args.range:
        git_range = args.range
        tags = git_range.split("..")
        if len(tags) == 2:
            latest = tags[1]
        else:
            latest = tags[0]
            tags_raw = run_cmd("git tag --sort=-v:refname")
            all_tags = tags_raw.splitlines() if tags_raw else []
            if latest in all_tags:
                idx = all_tags.index(latest)
                if idx + 1 < len(all_tags):
                    git_range = f"{all_tags[idx+1]}..{latest}"
                else:
                    git_range = latest
            else:
                latest = run_cmd("git tag --sort=-v:refname | head -n 1") or "HEAD"
    else:
        tags_raw = run_cmd("git tag --sort=-v:refname")
        tags = tags_raw.splitlines() if tags_raw else []
        if len(tags) < 2:
            latest = tags[0] if tags else "HEAD"
            git_range = "HEAD"
        else:
            latest = tags[0]
            git_range = f"{tags[1]}..{tags[0]}"

    print(f"Using range: {git_range}")

    log_output = run_cmd(f'git log --pretty=format:"- %s" {git_range}')
    
    sections = {
        'feat': [],
        'fix': [],
        'improve': [],
        'docs': []
    }

    for line in log_output.splitlines():
        if "conductor" in line.lower() or "checkpoint" in line.lower():
            continue
        
        match = re.match(r'- (\w+)(\([^)]+\))?:\s*(.*)', line)
        if not match:
            continue
            
        type_str, scope, msg = match.groups()
        msg = msg.strip()
        msg = msg[0].upper() + msg[1:] if msg else msg
        
        if scope:
            scope = scope[1:-1] # remove parens
            entry = f"- `{scope}`: {msg}"
        else:
            entry = f"- {msg}"

        if type_str == 'feat':
            sections['feat'].append(entry)
        elif type_str == 'fix':
            sections['fix'].append(entry)
        elif type_str in ['refactor', 'perf', 'style']:
            sections['improve'].append(entry)
        elif type_str == 'docs':
            sections['docs'].append(entry)

    summary = []
    if sections['feat']:
        summary.append("### 🚀 **New Features**\n\n" + "\n".join(sections['feat']) + "\n")
    if sections['fix']:
        summary.append("### 🐛 **Bug Fixes**\n\n" + "\n".join(sections['fix']) + "\n")
    if sections['improve']:
        summary.append("### ✨ **Improvements**\n\n" + "\n".join(sections['improve']) + "\n")
    if sections['docs']:
        summary.append("### 📝 **Documentation**\n\n" + "\n".join(sections['docs']) + "\n")

    thanks_entry = format_special_thanks(args.thanks, args.thanks_reason)
    if thanks_entry:
        summary.append(f"### 🙏 **Special Thanks**\n\n{thanks_entry}\n")
        
    url_range = git_range.replace("..", "...")
    if git_range == "HEAD" and latest == "HEAD":
        summary.append("**Full Changelog**: https://github.com/nicholaswilde/cyd-wifi-analyzer/commits/main\n")
    elif git_range == "HEAD":
        summary.append(f"**Full Changelog**: https://github.com/nicholaswilde/cyd-wifi-analyzer/commits/{latest}\n")
    else:
        summary.append(f"**Full Changelog**: https://github.com/nicholaswilde/cyd-wifi-analyzer/compare/{url_range}\n")
    
    notes = "\n".join(summary)
    
    if args.dry_run:
        print("Dry run summary output:\n")
        print(notes)
        return

    with open('/tmp/release_notes.md', 'w') as f:
        f.write(notes)
        
    print(f"Updating draft release for {latest}...")
    try:
        url = run_cmd(f"gh release edit {latest} --draft -F /tmp/release_notes.md")
        print(f"Draft release updated: {url}")
    except Exception as e:
        print(f"Failed to update release: {e}")
    finally:
        if os.path.exists('/tmp/release_notes.md'):
            os.remove('/tmp/release_notes.md')

if __name__ == "__main__":
    main()
