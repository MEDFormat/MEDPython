
import json

with open('dhn_medlib/medlib_m12.h') as f:
    lines = f.readlines()

flag_dict = {
    'LH': {},
    'DM': {}
}

for line in lines:
    if line.startswith('#define'):
        mod_line = line.replace('#define ', '').replace('\n', '')
        mod_line = mod_line.lstrip()

        # Remove commens
        mod_line = mod_line.split('//')[0]

        for prefix in ['LH', 'DM']:
            if mod_line.startswith(prefix) and 'ui8' in mod_line and '<<' in mod_line:
                def_val = [x for x in mod_line.split('\t') if len(x)]
                val_str = def_val[1]
                pos_str = val_str.split('<<')[1]
                pos = int(pos_str.replace(')', '').strip())

                flag_dict[prefix][def_val[0]] = pos

json_string = json.dumps(flag_dict)

with open("src/dhn_med_py/medlib_flags.py", "w") as outfile:
    outfile.write(f'FLAGS = {json_string}')

