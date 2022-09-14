import pandas as pd
from sklearn import metrics
from sklearn.metrics import mean_squared_error as mse
import numpy as np

true_y = pd.read_csv('data/test_y.tsv', sep='\t', header=None,
                     names=['indiv', 'phen1', 'phen2', 'phen3', 'phen4', 'phen5'])
model_results = pd.DataFrame()
auc_total = 0
for i in [1, 2, 3, 4, 5]:
    model = pd.read_csv('output/out_model%d.csv' % i, header=None, names=['model%d' % i])
    model_results = pd.concat([model_results, model], axis=1)
    if (i == 4) or (i == 5):
        fpr, tpr, thresholds = metrics.roc_curve(
            true_y['phen%d' % i], model['model%d' % i], pos_label=1)
        auroc = round(metrics.auc(fpr, tpr), 3)
        print("Model%d AUROC score: " % i, auroc, flush=True)
        auc_total += auroc;
    else:
        ymin = min(true_y['phen%d' % i])
        ymax = max(true_y['phen%d' % i])
        normalize = ymax-ymin
        rmse = np.sqrt(mse(true_y['phen%d' % i], model['model%d' % i]))
        print("Model%d 1-NRMSE score: " % i, 1-rmse/normalize, flush=True)
        auc_total += 1-rmse/normalize

print("Sum of all scores:  ", auc_total)
model_results.to_csv('./all_models_final.csv',
                     sep='\t', header=True, index=False)
