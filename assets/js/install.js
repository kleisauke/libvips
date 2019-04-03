const downloadPrefix = 'https://github.com/libvips/build-win64-mxe/releases/download/v';
const latestRelease = 'https://api.github.com/repos/libvips/build-win64-mxe/releases/latest';
let latestVersion;

function update_links(variant, architecture, linking) {
  const suffix = linking === 'shared' ? '' : `-${linking}`;
  const binary = `vips-dev-${architecture}-${variant}-${latestVersion}${suffix}.zip`;

  const downloadEl = document.getElementById('download');
  downloadEl.href = `${downloadPrefix}${latestVersion}/${binary}`;
  downloadEl.title = binary;
}

function update_states(variant, architecture, linking) {
  [variant, architecture, linking].forEach((element) => {
    const input = document.querySelector(`input[value='${element}']`);
    const button = input.parentElement;
    for (const sibling of button.parentElement.children) {
      sibling.classList.remove('active');
    }
    button.classList.add('active');
  });
}

async function init() {
  // getHighEntropyValues() is Chrome-only; default to 64-bit Windows binaries otherwise
  let { architecture = 'x86', bitness = '64' } =
    await navigator.userAgentData?.getHighEntropyValues?.(["architecture", "bitness"]) ?? {};

  switch (architecture) {
    case 'x86':
      architecture = bitness === '64' ? 'w64' : 'w32';
      break;
    case 'arm':
      architecture = 'arm64';
      break;
    default:
      architecture = 'w64';
      break;
  }

  const variant = 'web';
  const linking = architecture === 'arm64' ? 'static' : 'shared';

  const input = document.querySelector(`input[value='${architecture}']`);
  input.checked = true;
  
  const response = await fetch(latestRelease);
  if (!response.ok) {
    throw new Error(`HTTP error! Status: ${response.status}`);
  }

  const json = await response.json();
  latestVersion = json.tag_name.slice(1);

  update_states(variant, architecture, linking);
  update_links(variant, architecture, linking);
}

await init();

function checked_val(name) {
  return document.querySelector(`input[name='${name}']:checked`).value;
}

document.querySelectorAll('input').forEach(el => {
  el.addEventListener('change', () => {
    const architecture = checked_val('architecture');
    let variant = checked_val('variant');
    let linking = checked_val('linking');

    const staticLinkInputs = document.querySelectorAll("input[value^='static']");
    if (variant === 'all') {
      // Distributing statically linked binaries against GPL libraries, violates the GPL license.
      if (linking.startsWith('static')) {
        staticLinkInputs.forEach(e => e.checked = false);
        linking = 'shared';
        document.querySelector(`input[value='${linking}']`).checked = true;
      }

      staticLinkInputs.forEach(e => e.parentElement.classList.add('disabled'));

      // -all variant is not available for Windows Arm64.
      document.querySelector("input[value='arm64']").parentElement.classList.add('disabled');
    } else {
      staticLinkInputs.forEach(e => e.parentElement.classList.remove('disabled'));
      document.querySelector("input[value='arm64']").parentElement.classList.remove('disabled');

      // Only the static(-ffi) variants are available for Windows Arm64.
      if (architecture === 'arm64') {
        if (linking === 'shared') {
          document.querySelector(`input[value='${linking}']`).checked = false;
          linking = 'static';
          document.querySelector(`input[value='${linking}']`).checked = true;
        }

        document.querySelector("input[value='all']").parentElement.classList.add('disabled');
        document.querySelector("input[value='shared']").parentElement.classList.add('disabled');
      } else {
        document.querySelector("input[value='all']").parentElement.classList.remove('disabled');
        document.querySelector("input[value='shared']").parentElement.classList.remove('disabled');
      }
    }

    update_states(variant, architecture, linking);
    update_links(variant, architecture, linking);
  })
});
