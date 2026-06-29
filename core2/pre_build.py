import os
import shutil

Import("env")

project_dir = env.get("PROJECT_DIR", ".")
csv_path = os.path.join(project_dir, "data", "questions.csv")
template_path = os.path.join(project_dir, "data", "questions.csv.template")

# If questions.csv is missing (e.g. fresh clone), copy from template
if not os.path.exists(csv_path) and os.path.exists(template_path):
    print("Auto-generating data/questions.csv from template for build...")
    os.makedirs(os.path.dirname(csv_path), exist_ok=True)
    shutil.copy(template_path, csv_path)
