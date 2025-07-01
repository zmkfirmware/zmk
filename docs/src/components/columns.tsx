import { ReactNode, CSSProperties } from "react";
import clsx from "clsx";

interface ColumnsProps {
  children: ReactNode;
  className?: string;
  style?: CSSProperties;
}
// className will allow you to pass either your custom classes or the native infima classes https://infima.dev/docs/layout/grid.
// style will allow you to either pass your custom styles directly, which can be an alternative to the "styles.module.css" file in certain cases.
export function Columns({ children, className, style }: ColumnsProps) {
  return (
    <div className="container center">
      <div className={clsx("row", className)} style={style}>
        {children}
      </div>
    </div>
  );
}

interface ColumnProps {
  children: ReactNode;
  className?: string;
  style?: CSSProperties;
}
export function Column({ children, className, style }: ColumnProps) {
  return (
    <div className={clsx("col", className)} style={style}>
      {children}
    </div>
  );
}
