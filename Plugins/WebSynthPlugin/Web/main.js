// The page half of the editor. It talks to the plugin through window.eacp,
// which eacp's WebViewBridge injects before this script runs:
//
//   eacp.invoke('getParameters')           what the parameters are, once
//   eacp.invoke('beginGesture', {id})      a drag starts
//   eacp.invoke('setParameter', {id, value})
//   eacp.invoke('endGesture', {id})        a drag ends
//   eacp.on('parameterChanged', ...)       any parameter moved, by anyone
//   eacp.on('midiActivity', ...)           keys held, last note
//
// Values are normalised, 0 to 1, in both directions. The text shown beside a
// control is always the plugin's own, carried in with the value.

(function () {
    'use strict';

    const bridge = window.eacp;

    // id -> {info, render(value, text)} for every control on the page.
    const controls = new Map();

    // The parameter under the pointer right now, if any. Its position is ours
    // until the pointer lets go: the echo of an older value from the plugin
    // updates its readout but not where the knob points.
    let dragging = null;

    const readout = document.getElementById('readout');

    // The parameter the footer is showing, so a change to it — a click on the
    // control the pointer is over, or automation — refreshes the footer too.
    let readoutId = null;

    function showReadout(info) {
        readoutId = info.id;
        readout.textContent = info.name + '  ' + info.text;
    }

    // Parameter traffic ----------------------------------------------------

    function beginGesture(id) {
        bridge.invoke('beginGesture', {id: id});
    }

    function setValue(id, value) {
        bridge.invoke('setParameter', {id: id, value: value});
    }

    function endGesture(id) {
        bridge.invoke('endGesture', {id: id});
    }

    // A click is a whole gesture: the host records one touch, one value, one
    // release. The commands are dispatched in order, so the three are too.
    function setAsCompleteGesture(id, value) {
        beginGesture(id);
        setValue(id, value);
        endGesture(id);
    }

    function update(id, value, text) {
        const control = controls.get(id);

        if (!control)
            return;

        control.info.text = text;

        if (dragging !== id)
            control.info.value = value;

        control.render();

        if (readoutId === id)
            showReadout(control.info);
    }

    // Knobs ---------------------------------------------------------------

    const svgNS = 'http://www.w3.org/2000/svg';
    const sweep = 135; // degrees either side of straight up

    function point(degrees, radius) {
        const radians = degrees * Math.PI / 180;
        return [50 + radius * Math.sin(radians), 50 - radius * Math.cos(radians)];
    }

    function arcPath(from, to, radius) {
        const [x0, y0] = point(from, radius);
        const [x1, y1] = point(to, radius);
        const large = to - from > 180 ? 1 : 0;
        return 'M ' + x0 + ' ' + y0 + ' A ' + radius + ' ' + radius
               + ' 0 ' + large + ' 1 ' + x1 + ' ' + y1;
    }

    function svgElement(name, attributes) {
        const element = document.createElementNS(svgNS, name);

        for (const key in attributes)
            element.setAttribute(key, attributes[key]);

        return element;
    }

    function makeKnob(element, info) {
        const svg = svgElement('svg', {viewBox: '0 0 100 100'});
        const track = svgElement('path', {class: 'track', d: arcPath(-sweep, sweep, 42)});
        const fill = svgElement('path', {class: 'fill'});
        const cap = svgElement('circle', {class: 'cap', cx: 50, cy: 50, r: 31});
        const pointer = svgElement('line', {class: 'pointer'});

        svg.append(track, fill, cap, pointer);

        const name = document.createElement('div');
        name.className = 'knob-name';
        name.textContent = info.name;

        const value = document.createElement('div');
        value.className = 'knob-value';

        element.append(svg, name, value);

        function render() {
            const angle = -sweep + info.value * 2 * sweep;

            // A zero-length arc draws a dot under the round cap; skip it.
            fill.setAttribute('d', info.value > 0.001 ? arcPath(-sweep, angle, 42) : '');

            const [x0, y0] = point(angle, 12);
            const [x1, y1] = point(angle, 26);
            pointer.setAttribute('x1', x0);
            pointer.setAttribute('y1', y0);
            pointer.setAttribute('x2', x1);
            pointer.setAttribute('y2', y1);

            value.textContent = info.text;
        }

        // Vertical drag, 200 pixels end to end, ten times finer with Shift.
        let startY = 0;
        let startValue = 0;

        element.addEventListener('pointerdown', (event) => {
            if (event.button !== 0)
                return;

            element.setPointerCapture(event.pointerId);
            element.classList.add('dragging');

            dragging = info.id;
            startY = event.clientY;
            startValue = info.value;

            beginGesture(info.id);
            showReadout(info);
        });

        element.addEventListener('pointermove', (event) => {
            if (dragging !== info.id)
                return;

            const range = event.shiftKey ? 2000 : 200;
            const next = Math.min(1, Math.max(0, startValue + (startY - event.clientY) / range));

            if (next === info.value)
                return;

            info.value = next;
            render();
            setValue(info.id, next);
        });

        function release(event) {
            if (dragging !== info.id)
                return;

            element.releasePointerCapture(event.pointerId);
            element.classList.remove('dragging');
            dragging = null;
            endGesture(info.id);
        }

        element.addEventListener('pointerup', release);
        element.addEventListener('pointercancel', release);

        element.addEventListener('dblclick', () => {
            setAsCompleteGesture(info.id, info.defaultValue);
        });

        element.addEventListener('wheel', (event) => {
            event.preventDefault();
            const step = (event.shiftKey ? 0.002 : 0.02) * Math.sign(-event.deltaY);
            setAsCompleteGesture(info.id, Math.min(1, Math.max(0, info.value + step)));
        }, {passive: false});

        element.addEventListener('pointerenter', () => showReadout(info));

        return render;
    }

    // Waveform buttons ------------------------------------------------------

    // One small drawing per choice, looked up by the choice's own name so the
    // plugin stays the authority on which choices there are and in what order.
    const waveGlyphs = {
        Sine: 'M 2 8 C 6 -2, 11 -2, 17 8 S 28 18, 32 8',
        Triangle: 'M 2 8 L 9 2 L 23 14 L 32 6',
        Saw: 'M 2 14 L 17 2 L 17 14 L 32 2 L 32 14',
        Square: 'M 2 14 L 2 2 L 17 2 L 17 14 L 32 14 L 32 2'
    };

    function makeChoiceButtons(element, info) {
        const last = Math.max(1, info.choices.length - 1);

        const buttons = info.choices.map((choice, index) => {
            const button = document.createElement('button');
            button.className = 'wave';

            const glyph = svgElement('svg', {viewBox: '0 0 34 16'});
            glyph.append(svgElement('path', {d: waveGlyphs[choice] || ''}));

            const label = document.createElement('span');
            label.textContent = choice;

            button.append(glyph, label);
            button.addEventListener('click', () => setAsCompleteGesture(info.id, index / last));
            button.addEventListener('pointerenter', () => showReadout(info));

            element.append(button);
            return button;
        });

        return function render() {
            const selected = Math.round(info.value * last);
            buttons.forEach((button, index) =>
                button.classList.toggle('selected', index === selected));
        };
    }

    // Toggle --------------------------------------------------------------

    function makeToggle(element, info) {
        element.addEventListener('click', () =>
            setAsCompleteGesture(info.id, info.value >= 0.5 ? 0 : 1));

        element.addEventListener('pointerenter', () => showReadout(info));

        return function render() {
            element.classList.toggle('on', info.value >= 0.5);
        };
    }

    // MIDI activity -------------------------------------------------------

    const noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];

    function noteName(note) {
        return noteNames[note % 12] + (Math.floor(note / 12) - 1);
    }

    const midi = document.getElementById('midi');
    const midiText = midi.querySelector('.midi-text');

    function showMidi(activity) {
        midi.classList.toggle('active', activity.heldNotes > 0);

        if (activity.lastNote < 0)
            midiText.textContent = 'No notes';
        else if (activity.heldNotes > 0)
            midiText.textContent = noteName(activity.lastNote) + '  ·  '
                                   + activity.heldNotes + ' held';
        else
            midiText.textContent = noteName(activity.lastNote);
    }

    // Start-up ------------------------------------------------------------

    function build(parameters) {
        for (const info of parameters) {
            const element = document.querySelector('[data-param="' + info.id + '"]');

            if (!element)
                continue;

            let render;

            if (element.classList.contains('knob'))
                render = makeKnob(element, info);
            else if (element.classList.contains('waves'))
                render = makeChoiceButtons(element, info);
            else
                render = makeToggle(element, info);

            controls.set(info.id, {info: info, render: render});
            render();
        }
    }

    if (!bridge) {
        console.error('eacp bridge missing: this page is meant to run inside the plugin');
        return;
    }

    // Listen before asking, so a change landing between the request and the
    // reply is not lost: an update for a control not built yet is dropped, and
    // the list that builds it is newer than that update anyway.
    bridge.on('parameterChanged', (u) => update(u.id, u.value, u.text));
    bridge.on('midiActivity', showMidi);

    bridge.invoke('getParameters', {}).then((list) => build(list.parameters));
})();
