function example_caspase8()
%%
% COMPILATION

[exdir,~,~]=fileparts(which('example_caspase8.m'));
% compile the model
amiwrap('model_caspase8','model_caspase8_syms',exdir)

%%
% SIMULATION event triggered

% time vector
t = linspace(0,200,40);
p = [1.1901; 1.1853; -7.3811; -6.2073; -8.1331; -5.0799; -8.6386; -6.3326;...
    -3.6456; -8.6753; -6.5605; 4.4800; 4.6316; 4.6604; 4.4292; 13.5632;...
    10.7896; 10.3196; -2.0441; -0.6741; -0.7477];
k = 1.66;

options = amioption;
options.atol = 1e-8;
options.rtol = 1e-12;
options.nmaxevent = 1;
options.maxsteps = 1e5;
options.sensi = 1;

% load mex into memory
[~] = which('simulate_model_caspase8'); % fix for inaccessability problems
D = []; %amidata(length(t), 6, 1, 1, 1);
sol = simulate_model_caspase8(t, p, k, D, options);
disp(['Event triggered, time of event:' num2str(sol.z)])
disp(['Value of trigger function:' num2str(sol.rz)])

%% plot
figure()
plot(sol.t, sol.y(:, 6)); hold on;
plot([sol.z, sol.z], [min(sol.y(:, 6)) - 10, max(sol.y(:,6)) + 10], '--');
ylim([min(sol.y(:, 6)) - 10, max(sol.y(:,6)) + 10]);
legend('event function', 'time of event')
xlabel('time')
ylabel('tBID-BID_0*tBID_tapt')
title('event triggered')
%%
% SIMULATION event NOT triggered

% time vector
p_mod = p;
p_mod(10) = p_mod(10) - 10;

% load mex into memory
[~] = which('simulate_model_caspase8'); % fix for inaccessability problems
sol = simulate_model_caspase8(t, p_mod, k, D, options);
disp(['Event triggered, time of event:' num2str(sol.z)])
disp(['Value of trigger function:' num2str(sol.rz)])

%% plot
figure()
plot(sol.t, sol.y(:, 6));
legend('event function')
xlabel('time')
ylabel('tBID-BID_0*tBID_tapt')
title('event NOT triggered')