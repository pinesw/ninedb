import fs from 'fs';
import os from 'os';
import fetch from 'node-fetch';
import pjson from './package.json' with {type: 'json'};

let platform;
switch (os.platform()) {
    case 'win32':
        platform = 'windows';
        break;
    case 'darwin':
        platform = 'macos';
        break;
    case 'linux':
        platform = 'linux';
        break;
    default:
        throw new Error('Unsupported platform');
}

const arch = os.arch();
const node_version = process.versions.node.split('.')[0];

const url_template = pjson.binary.url;
const url = url_template
    .replace('{package_version}', pjson.version)
    .replace('{platform}', platform)
    .replace('{arch}', arch)
    .replace('{node_version}', node_version);

console.log(`Downloading binary from ${url}`);

fetch(url)
    .then((res) => {
        if (!res.ok) {
            throw new Error(`Failed to download binary: ${res.statusText}`);
        }

        res.body.pipe(fs.createWriteStream(pjson.binary.target));
    });
