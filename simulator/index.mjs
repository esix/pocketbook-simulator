import { initApp } from './inkview-emu.mjs';

var PROJECTS = ['demo01', 'calc'];
var hash = location.hash.slice(1);
var project = PROJECTS.indexOf(hash) >= 0 ? hash : 'calc';

initApp(project);
